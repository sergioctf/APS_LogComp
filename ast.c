// ast.c
#include "ast.h"
#include <stdlib.h>
#include <string.h>

// Helpers internos
static Expr *new_expr(void) {
    Expr *e = malloc(sizeof *e);
    if (!e) exit(1);
    e->next = NULL;
    return e;
}

static Stmt *new_stmt(void) {
    Stmt *s = malloc(sizeof *s);
    if (!s) exit(1);
    s->next = NULL;
    return s;
}

// Construtores de Expr

Expr *make_int_expr(int v) {
    Expr *e = new_expr();
    e->kind = EXPR_INT;
    e->ival = v;
    return e;
}

Expr *make_float_expr(double v) {
    Expr *e = new_expr();
    e->kind = EXPR_FLOAT;
    e->fval = v;
    return e;
}

Expr *make_text_expr(char *s) {
    Expr *e = new_expr();
    e->kind = EXPR_TEXT;
    e->sval = s;
    return e;
}

Expr *make_cell_expr(char *cell) {
    Expr *e = new_expr();
    e->kind = EXPR_CELL;
    e->sval = cell;
    return e;
}

Expr *make_binary_expr(BinaryOp op, Expr *l, Expr *r) {
    Expr *e = new_expr();
    e->kind = EXPR_BINARY;
    e->bin.op    = op;
    e->bin.left  = l;
    e->bin.right = r;
    return e;
}

Expr *make_unary_expr(UnaryOp op, Expr *sub) {
    Expr *e = new_expr();
    e->kind    = EXPR_UNARY;
    e->un.op   = op;
    e->un.sub  = sub;
    return e;
}

Expr *make_call_expr(const char *fname, Expr *args) {
    Expr *e = new_expr();
    e->kind         = EXPR_CALL;
    e->call.fname   = strdup(fname);
    e->call.args    = args;
    return e;
}

Expr *make_range_expr(char *start, char *end) {
    Expr *e = new_expr();
    e->kind = EXPR_RANGE;
    e->range.start_cell = start;
    e->range.end_cell   = end;
    return e;
}

// Construtores de Stmt

Stmt *make_assign_stmt(char *cell, Expr *expr) {
    Stmt *s = new_stmt();
    s->kind = STMT_ASSIGN;
    s->assign.cell = cell;
    s->assign.expr = expr;
    return s;
}

Stmt *make_if_stmt(Expr *cond, Stmt *then_br) {
    Stmt *s = new_stmt();
    s->kind     = STMT_IF;
    s->ifs.cond = cond;
    s->ifs.then_branch = then_br;
    return s;
}

Stmt *make_while_stmt(Expr *cond, Stmt *body) {
    Stmt *s = new_stmt();
    s->kind        = STMT_WHILE;
    s->whiles.cond = cond;
    s->whiles.body = body;
    return s;
}

Stmt *make_table_stmt(void) {
    Stmt *s = new_stmt();
    s->kind = STMT_TABLE;
    return s;
}

Stmt *make_export_stmt(char *filename) {
    Stmt *s = new_stmt();
    s->kind       = STMT_EXPORT;
    s->exp.filename = filename;
    return s;
}

// Append em listas

Expr *expr_append(Expr *list, Expr *e) {
    if (!list) return e;
    Expr *it = list;
    while (it->next) it = it->next;
    it->next = e;
    return list;
}

Stmt *stmt_append(Stmt *list, Stmt *s) {
    if (!list) return s;
    Stmt *it = list;
    while (it->next) it = it->next;
    it->next = s;
    return list;
}
