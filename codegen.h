#ifndef LANGCELL_CODEGEN_H
#define LANGCELL_CODEGEN_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa o LLVM (cria o module e o execution engine)
void init_llvm(const char *module_name);

// Gera o IR a partir da AST inteira
void generate_code(Stmt *program);

// Executa a função 'main' JIT-compiled e retorna o seu valor
int run_code(void);

#ifdef __cplusplus
}
#endif

#endif // LANGCELL_CODEGEN_H
