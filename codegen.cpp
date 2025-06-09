// codegen.cpp
#include "codegen.h"
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
using namespace llvm;
using namespace llvm::orc;

CodeGenContext::CodeGenContext()
  : module(std::make_unique<Module>("LangCellJIT", context)),
    builder(context),
    jit(nullptr)
{}

// Gera a função 'main' e executa
void CodeGenContext::generateCode(Stmt* root) {
  Function* mainFn = createMain(root);
  verifyFunction(*mainFn);
  // registrar no JIT
  auto TSM = ThreadSafeModule(std::move(module), std::make_unique<LLVMContext>(context));
  LLJITBuilder builder;
  auto J = builder.create();
  jit = J->get();
  jit->addIRModule(std::move(TSM));
}

// Roda via JIT
void CodeGenContext::run() {
  auto sym = jit->lookup("main");
  auto mainPtr = (int (*)())sym.get().getAddress();
  int result = mainPtr();
  (void)result;
}

// Cria a função main que percorre todos os statements
Function* CodeGenContext::createMain(Stmt* root) {
  // void main()
  FunctionType* ftype = FunctionType::get(builder.getInt32Ty(), {}, false);
  Function* mainFn = Function::Create(ftype, Function::ExternalLinkage, "main", module.get());
  BasicBlock* entry = BasicBlock::Create(context, "entry", mainFn);
  builder.SetInsertPoint(entry);

  // gerar cada stmt
  for (Stmt* s = root; s; s = s->next)
    genStmt(s);

  builder.CreateRet(builder.getInt32(0));
  return mainFn;
}

// Gera código para um statement
void CodeGenContext::genStmt(Stmt* s) {
  switch (s->kind) {
    case STMT_ASSIGN: {
      // cria variável ou usa existente
      auto val = genExpr(s->assign.expr);
      cellVars[s->assign.cell] = val;
      break;
    }
    case STMT_TABLE:
      // gerar loop para imprimir todas as cellVars
      break;
    case STMT_EXPORT:
      // gerar chamadas para gravar CSV
      break;
    case STMT_IF:
      // gerar bloco condicional
      break;
    case STMT_WHILE:
      // gerar loop while
      break;
  }
}

// Gera uma expressão
Value* CodeGenContext::genExpr(Expr* e) {
  switch (e->kind) {
    case EXPR_INT:
      return builder.getInt32(e->ival);
    case EXPR_FLOAT:
      return ConstantFP::get(builder.getDoubleTy(), e->fval);
    case EXPR_CELL:
      return cellVars[e->sval];
    case EXPR_BINARY: {
      auto L = genExpr(e->bin.left);
      auto R = genExpr(e->bin.right);
      switch (e->bin.op) {
        case OP_ADD: return builder.CreateAdd(L,R);
        // … demais ops …
      }
    }
    case EXPR_CALL: {
      // Expandir SUM, AVERAGE, etc.
    }
    case EXPR_RANGE:
      // expandir para loop ou array de valores
    default:
      return nullptr;
  }
}
