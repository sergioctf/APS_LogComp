// ast.h
#ifndef LANGCELL_AST_H
#define LANGCELL_AST_H

#include <stdlib.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_TEXT,
    TYPE_ERROR
} Type;

// Kind de expressão
typedef enum {
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_TEXT,
    EXPR_CELL,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_RANGE     // Adicionado para A1:B2
} ExprKind;

// Operadores binários
typedef enum {
    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV,
    OP_GT,  OP_LT,
    OP_GE,  OP_LE,
    OP_EQ,  OP_NE,
    OP_AND, OP_OR
} BinaryOp;

// Operadores unários
typedef enum {
    OP_NEG,    // -x
    OP_NOT     // NOT x
} UnaryOp;

typedef struct Expr {
    ExprKind kind;
    union {
        int      ival;      // EXPR_INT
        double   fval;      // EXPR_FLOAT
        char    *sval;      // EXPR_TEXT, EXPR_CELL

        struct {               // EXPR_BINARY
            BinaryOp op;
            struct Expr *left, *right;
        } bin;

        struct {               // EXPR_UNARY
            UnaryOp op;
            struct Expr *sub;
        } un;

        struct {               // EXPR_CALL
            char      *fname;
            struct Expr *args;    // lista encadeada em .next
        } call;

        struct {               // EXPR_RANGE
            char *start_cell;
            char *end_cell;
        } range;
    };
    struct Expr *next;      // para listas (expression_list)
} Expr;

// Kind de statement
typedef enum {
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_TABLE,
    STMT_EXPORT
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    union {
        struct {                // STMT_ASSIGN
            char *cell;
            Expr *expr;
        } assign;
        struct {                // STMT_IF
            Expr *cond;
            struct Stmt *then_branch;
        } ifs;
        struct {                // STMT_WHILE
            Expr *cond;
            struct Stmt *body;
        } whiles;
        struct {                // STMT_EXPORT
            char *filename;
        } exp;
    };
    struct Stmt *next;      // sequência
} Stmt;

// Construtores de Expr
Expr *make_int_expr(int v);
Expr *make_float_expr(double v);
Expr *make_text_expr(char *s);
Expr *make_cell_expr(char *cell);
Expr *make_binary_expr(BinaryOp op, Expr *l, Expr *r);
Expr *make_unary_expr(UnaryOp op, Expr *sub);
Expr *make_call_expr(const char *fname, Expr *args);
Expr *make_range_expr(char *start, char *end);

// Construtores de Stmt
Stmt *make_assign_stmt(char *cell, Expr *e);
Stmt *make_if_stmt(Expr *cond, Stmt *then_br);
Stmt *make_while_stmt(Expr *cond, Stmt *body);
Stmt *make_table_stmt(void);
Stmt *make_export_stmt(char *filename);

// Funções de append
Expr *expr_append(Expr *list, Expr *e);
Stmt *stmt_append(Stmt *list, Stmt *s);

#endif // LANGCELL_AST_H
