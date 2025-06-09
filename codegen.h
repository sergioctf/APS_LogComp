// codegen.h
#ifndef LANGCELL_CODEGEN_H
#define LANGCELL_CODEGEN_H

#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

class CodeGenContext {
public:
  llvm::LLVMContext   context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<>   builder;
  std::map<std::string, llvm::Value*> cellVars;

  CodeGenContext();
  void generateCode(Stmt* root);
  void run();
private:
  llvm::orc::LLJIT*   jit;
  llvm::Function*     createMain(Stmt* root);
  llvm::Value*        genExpr(Expr* e);
  void                genStmt(Stmt* s);
};

#endif // LANGCELL_CODEGEN_H
