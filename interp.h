// interp.h
#ifndef LANGCELL_INTERP_H
#define LANGCELL_INTERP_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

int interpret(Stmt *program);

#ifdef __cplusplus
}
#endif

#endif // LANGCELL_INTERP_H
