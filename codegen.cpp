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
#include "llvm/IR/Type.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;

// Contexto e objetos globais do LLVM
static LLVMContext TheContext;
static Module     *TheModuleRaw    = nullptr;    // ponteiro bruto para o módulo JIT
static IRBuilder<> Builder(TheContext);
static ExecutionEngine *TheExecutionEngine = nullptr;

// Mapa de células para globals no IR
static std::map<std::string, GlobalVariable*> CellMap;

// Obtém ou cria uma GlobalVariable double para 'name'
static GlobalVariable* getCellGV(const std::string &name) {
    auto it = CellMap.find(name);
    if (it != CellMap.end())
        return it->second;

    llvm::Type *doubleTy = llvm::Type::getDoubleTy(TheContext);
    auto *GV = new GlobalVariable(
        *TheModuleRaw,        // módulo
        doubleTy,             // tipo
        false,                // não é constante
        GlobalValue::ExternalLinkage,
        ConstantFP::get(doubleTy, 0.0),
        name                  // nome da GV
    );
    CellMap[name] = GV;
    return GV;
}

// Gera IR para uma expressão (tudo como double)
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
        // CreateLoad(elementType, ptr, name)
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
        // retornamos 0.0 em casos não suportados
        return ConstantFP::get(
            llvm::Type::getDoubleTy(TheContext),
            0.0
        );
    }
}

// Gera IR para cada atribuição na lista de Stmts
static void codegenStmtList(Stmt *s) {
    for (; s; s = s->next) {
        if (s->kind == STMT_ASSIGN) {
            GlobalVariable *GV = getCellGV(s->assign.cell);
            Value *val = codegenExpr(s->assign.expr);
            Builder.CreateStore(val, GV);
        }
        // IF, WHILE, TABLE, EXPORT: pendentes de implementação
    }
}

void init_llvm(const char *module_name) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // Cria o módulo e guarda o raw pointer
    auto M_up = std::make_unique<Module>(module_name, TheContext);
    TheModuleRaw = M_up.get();

    // Cria o ExecutionEngine, movendo o módulo
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

void generate_code(Stmt *program) {
    // Função double main()
    llvm::Type *doubleTy = llvm::Type::getDoubleTy(TheContext);
    FunctionType *FT = FunctionType::get(doubleTy, false);
    Function *MainF = Function::Create(
        FT,
        Function::ExternalLinkage,
        "main",
        TheModuleRaw    // use o raw pointer aqui
    );

    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", MainF);
    Builder.SetInsertPoint(BB);

    // Gera as atribuições
    codegenStmtList(program);

    // Retorna 0.0
    Builder.CreateRet(ConstantFP::get(doubleTy, 0.0));
}

int run_code() {
    // Executa main() via JIT
    Function *F = TheExecutionEngine->FindFunctionNamed("main");
    std::vector<GenericValue> noargs;
    GenericValue gv = TheExecutionEngine->runFunction(F, noargs);
    // Pega o valor double e converte pra int
    return (int)gv.DoubleVal;
}
