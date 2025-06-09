#ifndef LANGCELL_SEMA_H
#define LANGCELL_SEMA_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declarações — usam o 'Type' definido em ast.h
Type analyze_expr(Expr *e);
int  analyze_stmt_list(Stmt *stmts);

#ifdef __cplusplus
}
#endif

#endif // LANGCELL_SEMA_H
