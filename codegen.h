// codegen.h

#ifndef LANGCELL_CODEGEN_H
#define LANGCELL_CODEGEN_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_llvm(const char *module_name);
void generate_code(Stmt *program);
int  run_code(void);

#ifdef __cplusplus
}
#endif

#endif // LANGCELL_CODEGEN_H
