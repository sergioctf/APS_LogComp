#include "codegen.h"
#include "ast.h"

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>

// LLVM headers (GlobalVariable.h *antes* de usar GlobalVariable)
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"

using namespace llvm;

// Declare apenas uma vez, aqui:
static std::map<std::string, GlobalVariable*> CellMap;
static std::map<std::string, GlobalVariable*> TextMap;


// Helpers do runtime (implementados em interp.c) para funções de agregação e export
extern "C" double sum_helper(int n, double *vals);
extern "C" double avg_helper(int n, double *vals);
extern "C" double min_helper(int n, double *vals);
extern "C" double max_helper(int n, double *vals);
extern "C" void export_csv(const char *filename,
                           int n_cells, const char **names, double *values);


// ——— globals —————————————————————————————————————————————————————————————
static LLVMContext           TheContext;
static Module               *TheModuleRaw        = nullptr;
static IRBuilder<>           Builder(TheContext);
static ExecutionEngine      *TheExecutionEngine  = nullptr;
static FunctionCallee        PrintfFunc;

// ——— recupera/cria GlobalVariable para 'name' ——————————————————————————————
static GlobalVariable* getCellGV(const std::string &name) {
    auto it = CellMap.find(name);
    if (it != CellMap.end()) return it->second;

    // Desambigua Type do LLVM
    llvm::Type *dblTy = llvm::Type::getDoubleTy(TheContext);
    auto *GV = new GlobalVariable(
        *TheModuleRaw,
        dblTy,
        false,
        GlobalValue::ExternalLinkage,
        ConstantFP::get(dblTy, 0.0),
        name
    );
    CellMap[name] = GV;
    return GV;
}

static GlobalVariable* getCellTextGV(const std::string &name) {
  auto it = TextMap.find(name);
  if (it != TextMap.end()) return it->second;
  // i8* type
  llvm::Type *i8ptr = llvm::PointerType::get( llvm::Type::getInt8Ty(TheContext), 0 );
  auto *GV = new GlobalVariable(
    *TheModuleRaw,
    i8ptr,
    false,
    GlobalValue::ExternalLinkage,
    llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(i8ptr)),
    name + "_txt"
  );
  TextMap[name] = GV;
  return GV;
}


// ——— gera IR para expressões ——————————————————————————————————————————
static Value* codegenExpr(Expr *e) {
    switch (e->kind) {
      case EXPR_INT:
        return ConstantFP::get(
          llvm::Type::getDoubleTy(TheContext),
          (double)e->ival
        );
      case EXPR_FLOAT:
        return ConstantFP::get(
          llvm::Type::getDoubleTy(TheContext),
          e->fval
        );
      case EXPR_CELL: {
        GlobalVariable *GV = getCellGV(e->sval);
        return Builder.CreateLoad(
          GV->getValueType(),
          GV,
          e->sval
        );
      }
      case EXPR_BINARY: {
        llvm::Type *dblTy = llvm::Type::getDoubleTy(TheContext);
        Value *L = codegenExpr(e->bin.left);
        Value *R = codegenExpr(e->bin.right);
        Value *res = nullptr;
      
        switch (e->bin.op) {
          // aritmética
          case OP_ADD: res = Builder.CreateFAdd(L, R, "addtmp"); break;
          case OP_SUB: res = Builder.CreateFSub(L, R, "subtmp"); break;
          case OP_MUL: res = Builder.CreateFMul(L, R, "multmp"); break;
          case OP_DIV: res = Builder.CreateFDiv(L, R, "divtmp"); break;
      
          // comparadores → produzem i1, convertemos para double (1.0 / 0.0)
          case OP_GT: {
            Value *cmp = Builder.CreateFCmpOGT(L, R, "gtcmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
          case OP_LT: {
            Value *cmp = Builder.CreateFCmpOLT(L, R, "ltcmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
          case OP_GE: {
            Value *cmp = Builder.CreateFCmpOGE(L, R, "gecmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
          case OP_LE: {
            Value *cmp = Builder.CreateFCmpOLE(L, R, "lecmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
          case OP_EQ: {
            Value *cmp = Builder.CreateFCmpOEQ(L, R, "eqcmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
          case OP_NE: {
            Value *cmp = Builder.CreateFCmpONE(L, R, "necmp");
            res = Builder.CreateUIToFP(cmp, dblTy, "bool2dbl");
          } break;
      
          // lógicos → interpretamos 0/!=0
          case OP_AND: {
            Value *l1 = Builder.CreateFCmpONE(L, ConstantFP::get(dblTy, 0.0), "l1");
            Value *r1 = Builder.CreateFCmpONE(R, ConstantFP::get(dblTy, 0.0), "r1");
            Value *andv = Builder.CreateAnd(l1, r1, "andtmp");
            res = Builder.CreateUIToFP(andv, dblTy, "bool2dbl");
          } break;
          case OP_OR: {
            Value *l1 = Builder.CreateFCmpONE(L, ConstantFP::get(dblTy, 0.0), "l1");
            Value *r1 = Builder.CreateFCmpONE(R, ConstantFP::get(dblTy, 0.0), "r1");
            Value *orv = Builder.CreateOr(l1, r1, "ortmp");
            res = Builder.CreateUIToFP(orv, dblTy, "bool2dbl");
          } break;
      
          default:
            // Nunca deve chegar aqui para ops não reconhecidos
            res = ConstantFP::get(dblTy, 0.0);
        }
      
        return res;
      }
      
      case EXPR_TEXT: {
        // literal: criamos um GlobalStringPtr para o conteúdo
        llvm::Value *str = Builder.CreateGlobalStringPtr(
          e->sval,
          "strlit"
        );
        return str;  // é um i8*
      }      
      case EXPR_CALL: {
        // só cobrimos SUM, AVERAGE, MIN, MAX
        llvm::Type *dblTy = llvm::Type::getDoubleTy(TheContext);
        const std::string fname = e->call.fname;
        if (fname=="SUM" || fname=="AVERAGE" || fname=="MIN" || fname=="MAX") {
            // --- 1) conta quantos elementos irá empacotar em array ---
            int total = 0;
            for (Expr *arg = e->call.args; arg; arg = arg->next) {
                if (arg->kind==EXPR_RANGE) {
                    // A1:B2 => colA..colB e rowA..rowB
                    char sc = arg->range.start_cell[0], ec = arg->range.end_cell[0];
                    int  sr = atoi(arg->range.start_cell+1),
                         er = atoi(arg->range.end_cell+1);
                    total += (ec - sc + 1) * (er - sr + 1);
                } else {
                    total += 1;
                }
            }
    
            // --- 2) alloca array de double [total] no stack ---
            ArrayType *arrTy = ArrayType::get(dblTy, total);
            Value *arrAlloca = Builder.CreateAlloca(arrTy, nullptr, "argarr");
    
            // --- 3) preenche o array ---
            int idx = 0;
            for (Expr *arg = e->call.args; arg; arg = arg->next) {
                if (arg->kind==EXPR_RANGE) {
                    char sc = arg->range.start_cell[0], ec = arg->range.end_cell[0];
                    int  sr = atoi(arg->range.start_cell+1),
                         er = atoi(arg->range.end_cell+1);
                    for (char c = sc; c <= ec; ++c) {
                      for (int r = sr; r <= er; ++r) {
                        // nome da célula
                        char cell[16];
                        snprintf(cell, sizeof cell, "%c%d", c, r);
                        // load do GlobalVariable
                        GlobalVariable *GV = getCellGV(cell);
                        Value *val = Builder.CreateLoad(dblTy, GV);
                        // GEP para arrAlloca[idx]
                        Value *ptr = Builder.CreateConstGEP2_32(arrTy, arrAlloca, 0, idx);
                        Builder.CreateStore(val, ptr);
                        idx++;
                      }
                    }
                } else {
                    // expr individual
                    Value *val = codegenExpr(arg);
                    Value *ptr = Builder.CreateConstGEP2_32(arrTy, arrAlloca, 0, idx++);
                    Builder.CreateStore(val, ptr);
                }
            }
    
            // --- 4) chama o helper sum_helper/avg_helper/... ---
            // escolhe o nome correto do helper em C
            std::string helperName;
            if      (fname == "SUM")     helperName = "sum_helper";
            else if (fname == "AVERAGE") helperName = "avg_helper";
            else if (fname == "MIN")     helperName = "min_helper";
            else if (fname == "MAX")     helperName = "max_helper";

            // declara a função com o nome certo
            llvm::FunctionCallee helper = TheModuleRaw->getOrInsertFunction(
              helperName, 
              llvm::FunctionType::get(
                dblTy,
                { llvm::Type::getInt32Ty(TheContext),
                  llvm::PointerType::get(dblTy, 0) },
                false
              )
            );

            Value *countV = ConstantInt::get(
                              IntegerType::getInt32Ty(TheContext),
                              total
                            );
            // ptr do array: &arrAlloca[0]
            Value *dataPtr = Builder.CreateConstGEP2_32(arrTy, arrAlloca, 0, 0);
            return Builder.CreateCall(helper, { countV, dataPtr }, "callagg");
        }
    
        // Para outras chamadas caia no fallback (retorna 0)
        return ConstantFP::get(dblTy, 0.0);
    }
    
      default:
        return ConstantFP::get(
          llvm::Type::getDoubleTy(TheContext),
          0.0
        );
    }
}

// ——— gera IR para atribuições, IF, WHILE e EXPORT ———————————————————————————
static void codegenStmtList(Stmt *s, Function *F, BasicBlock *&BB) {
  for (; s; s = s->next) {
    Builder.SetInsertPoint(BB);

    // ASSIGN
    if (s->kind == STMT_ASSIGN) {
      if (s->assign.expr->kind == EXPR_TEXT) {
        // só texto
        auto *GVtxt = getCellTextGV(s->assign.cell);
        Value  *txt  = codegenExpr(s->assign.expr);
        Builder.CreateStore(txt, GVtxt);
      } else {
        // só numérico
        auto *GVnum = getCellGV(s->assign.cell);
        Value  *val  = codegenExpr(s->assign.expr);
        Builder.CreateStore(val, GVnum);
      }

    // IF
    } else if (s->kind == STMT_IF) {
      Value *condV = codegenExpr(s->ifs.cond);
      Value *cmp   = Builder.CreateFCmpONE(
        condV,
        ConstantFP::get(TheContext, APFloat(0.0)),
        "ifcond"
      );
      BasicBlock *thenBB = BasicBlock::Create(TheContext, "then",   F);
      BasicBlock *contBB = BasicBlock::Create(TheContext, "ifcont", F);
      Builder.CreateCondBr(cmp, thenBB, contBB);

      Builder.SetInsertPoint(thenBB);
      codegenStmtList(s->ifs.then_branch, F, thenBB);
      Builder.CreateBr(contBB);

      Builder.SetInsertPoint(contBB);
      BB = contBB;

    // WHILE
    } else if (s->kind == STMT_WHILE) {
      BasicBlock *condBB = BasicBlock::Create(TheContext, "while.cond", F);
      BasicBlock *bodyBB = BasicBlock::Create(TheContext, "while.body", F);
      BasicBlock *endBB  = BasicBlock::Create(TheContext, "while.end",  F);
      Builder.CreateBr(condBB);

      // condição
      Builder.SetInsertPoint(condBB);
      Value *condV2 = codegenExpr(s->whiles.cond);
      Value *cmp2   = Builder.CreateFCmpONE(
        condV2,
        ConstantFP::get(TheContext, APFloat(0.0)),
        "whilecond"
      );
      Builder.CreateCondBr(cmp2, bodyBB, endBB);

      // corpo
      Builder.SetInsertPoint(bodyBB);
      codegenStmtList(s->whiles.body, F, bodyBB);
      Builder.CreateBr(condBB);

      // fim
      Builder.SetInsertPoint(endBB);
      BB = endBB;

    // EXPORT
    } else if (s->kind == STMT_EXPORT) {
      // tipos comuns
      llvm::Type *dblTy = llvm::Type::getDoubleTy(TheContext);
      auto *i8ptr       = llvm::PointerType::get(llvm::Type::getInt8Ty(TheContext), 0);
      auto *FILEptr     = llvm::PointerType::get(llvm::StructType::create(TheContext, "struct._IO_FILE"), 0);

      // fopen(filename, "w")
      Value *fname = Builder.CreateGlobalStringPtr(s->exp.filename, "fname");
      Value *mode  = Builder.CreateGlobalStringPtr("w", "mode");
      auto fopenFn = TheModuleRaw->getOrInsertFunction(
        "fopen",
        FunctionType::get(FILEptr, { i8ptr, i8ptr }, false)
      );
      Value *file = Builder.CreateCall(fopenFn, { fname, mode }, "f");

      // declara fprintf e fclose
      auto fprintfFn = TheModuleRaw->getOrInsertFunction(
        "fprintf",
        FunctionType::get(
          IntegerType::getInt32Ty(TheContext),
          { FILEptr, i8ptr },
          true // varargs
        )
      );
      auto fcloseFn = TheModuleRaw->getOrInsertFunction(
        "fclose",
        FunctionType::get(IntegerType::getInt32Ty(TheContext), { FILEptr }, false)
      );

      // formatos
      Value *fmtNum = Builder.CreateGlobalStringPtr("%s,%g\n", "fmtNum");
      Value *fmtTxt = Builder.CreateGlobalStringPtr("%s,%s\n", "fmtTxt");

      // escreve numéricos
      for (auto &pr : CellMap) {
        Value *nm = Builder.CreateGlobalStringPtr(pr.first, "nm");
        Value *v  = Builder.CreateLoad(dblTy, pr.second);
        Builder.CreateCall(fprintfFn, { file, fmtNum, nm, v });
      }
      // escreve textos
      for (auto &pr : TextMap) {
        Value *nm  = Builder.CreateGlobalStringPtr(pr.first, "nm");
        GlobalVariable *GVtxt = pr.second;
        Value *txt = Builder.CreateLoad(i8ptr, GVtxt);
        Builder.CreateCall(fprintfFn, { file, fmtTxt, nm, txt });
      }
      // fecha
      Builder.CreateCall(fcloseFn, { file });
    }
  }
}


// ——— inicializa LLVM + MCJIT ——————————————————————————————————————————
void init_llvm(const char *module_name) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // adicionar essa linha:
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

  auto M_up = std::make_unique<Module>(module_name, TheContext);
  TheModuleRaw = M_up.get();

  // protótipo de printf — desambiguado
  PrintfFunc = TheModuleRaw->getOrInsertFunction(
    "printf",
    FunctionType::get(
      IntegerType::getInt32Ty(TheContext),
      PointerType::get( llvm::Type::getInt8Ty(TheContext), 0 ),
      /*isVarArg=*/true
    )
  );

  std::string err;
  TheExecutionEngine = EngineBuilder(std::move(M_up))
    .setErrorStr(&err)
    .setEngineKind(EngineKind::JIT)
    .create();
  if (!TheExecutionEngine) {
    std::fprintf(stderr, "Erro criando ExecutionEngine: %s\n", err.c_str());
    std::exit(1);
  }
}

// ——— monta o `main` + TABLE + verifica e finaliza JIT ——————————————————————
void generate_code(Stmt *program) {
  llvm::Type *doubleTy = llvm::Type::getDoubleTy(TheContext);
  FunctionType *FT = FunctionType::get(doubleTy, false);
  Function *MainF = Function::Create(
    FT,
    Function::ExternalLinkage,
    "main",
    TheModuleRaw
  );
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", MainF);
  Builder.SetInsertPoint(BB);

  codegenStmtList(program, MainF, BB);

  // imprime TABLE

  // 1) numéricos
  Value *fmtNum = Builder.CreateGlobalStringPtr("%s\t%g\n", "fmtNum");
  for (auto &pr : CellMap) {
    Value *nameStr = Builder.CreateGlobalStringPtr(pr.first, "cname");
    Value *val     = Builder.CreateLoad(
                       pr.second->getValueType(),
                       pr.second
                     );
    Builder.CreateCall(PrintfFunc, { fmtNum, nameStr, val });
  }

  // 2) textos
  Value *fmtText = Builder.CreateGlobalStringPtr("%s\t%s\n", "fmtText");
  for (auto &pr : TextMap) {
    Value *nameStr = Builder.CreateGlobalStringPtr(pr.first, "cnameTxt");
    Value *txtPtr  = Builder.CreateLoad(
                       pr.second->getValueType(),
                       pr.second
                     );
    Builder.CreateCall(PrintfFunc, { fmtText, nameStr, txtPtr });
  }

  Builder.CreateRet(ConstantFP::get(doubleTy, APFloat(0.0)));

  // verifica o módulo
  if (verifyModule(*TheModuleRaw, &errs())) {
    errs() << "=== ERRO de verificação do módulo ===\n";
    TheModuleRaw->print(outs(), nullptr);
    std::exit(1);
  }

  // finalize o JIT e mostre o IR
  TheExecutionEngine->finalizeObject();
  outs() << "===== IR gerado =====\n";
  TheModuleRaw->print(outs(), nullptr);
  outs() << "=====================\n";
}

// ——— executa o `main` compilado ———————————————————————————————————————
int run_code() {
  Function *F = TheExecutionEngine->FindFunctionNamed("main");
  std::vector<GenericValue> noargs;
  GenericValue gv = TheExecutionEngine->runFunction(F, noargs);
  return (int)gv.DoubleVal;
}
