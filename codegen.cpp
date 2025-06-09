// codegen.cpp

#include "codegen.h"
#include "ast.h"

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;

// ——— globals —————————————————————————————————————————————————————————————
static LLVMContext           TheContext;
static Module               *TheModuleRaw        = nullptr;
static IRBuilder<>           Builder(TheContext);
static ExecutionEngine      *TheExecutionEngine  = nullptr;
static FunctionCallee        PrintfFunc;
static std::map<std::string, GlobalVariable*> CellMap;

// ——— recupera/cria GlobalVariable para 'name' ——————————————————————————————
static GlobalVariable* getCellGV(const std::string &name) {
    auto it = CellMap.find(name);
    if (it != CellMap.end()) return it->second;

    // usa llvm::Type::getDoubleTy para não colidir com o enum Type
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

// ——— gera IR para expressões ——————————————————————————————————————————
static Value* codegenExpr(Expr *e) {
    switch (e->kind) {
      case EXPR_INT:
        return ConstantFP::get(
          // desambiguado
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
        Value *L = codegenExpr(e->bin.left);
        Value *R = codegenExpr(e->bin.right);
        switch (e->bin.op) {
          case OP_ADD: return Builder.CreateFAdd(L, R, "addtmp");
          case OP_SUB: return Builder.CreateFSub(L, R, "subtmp");
          case OP_MUL: return Builder.CreateFMul(L, R, "multmp");
          case OP_DIV: return Builder.CreateFDiv(L, R, "divtmp");
          default:     break;
        }
      }
      default:
        return ConstantFP::get(
          llvm::Type::getDoubleTy(TheContext),
          0.0
        );
    }
}

// ——— gera IR para atribuições, IF e WHILE ————————————————————————————————
static void codegenStmtList(Stmt *s, Function *F, BasicBlock *&BB) {
  for (; s; s = s->next) {
    Builder.SetInsertPoint(BB);

    if (s->kind == STMT_ASSIGN) {
      GlobalVariable *GV = getCellGV(s->assign.cell);
      Value *val = codegenExpr(s->assign.expr);
      Builder.CreateStore(val, GV);

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
    }
  }
}

// ——— inicializa LLVM + MCJIT ——————————————————————————————————————————
void init_llvm(const char *module_name) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  auto M_up = std::make_unique<Module>(module_name, TheContext);
  TheModuleRaw = M_up.get();

  // protótipo de printf — usando llvm::Type::getInt8Ty
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
  // também desambigua aqui
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
  Value *fmtStr = Builder.CreateGlobalStringPtr("%s\t%g\n", "fmt");
  for (auto &pr : CellMap) {
    Value *nameStr = Builder.CreateGlobalStringPtr(pr.first, "cname");
    Value *val     = Builder.CreateLoad(
                        pr.second->getValueType(),
                        pr.second,
                        pr.first + "_v"
                    );
    Builder.CreateCall(PrintfFunc, { fmtStr, nameStr, val });
  }

  Builder.CreateRet(ConstantFP::get(doubleTy, 0.0));

  // verifica o módulo
  if (verifyModule(*TheModuleRaw, &errs())) {
    errs() << "=== ERRO de verificação do módulo ===\n";
    TheModuleRaw->print(outs(), nullptr);
    std::exit(1);
  }

  // finalize JIT antes de executar
  TheExecutionEngine->finalizeObject();

  // dump opcional do IR
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
