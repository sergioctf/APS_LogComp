/* langcell.y */
 %{
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "ast.h"

 extern int yylex(void);
 extern int yyerror(const char *s);

 /* program_root é definido em main.cpp */
 extern Stmt *program_root;
 %}

/* Define START */
%start start

/* Tipos semânticos */
%union {
    int       ival;
    double    fval;
    char     *sval;
    Expr     *expr;
    Expr     *expr_list;
    Stmt     *stmt;
    Stmt     *stmt_list;
}

/* Tokens com valor */
%token  <ival>    INT
%token  <fval>    FLOAT
%token  <sval>    TEXT CELL

%token            IF THEN WHILE TABLE EXPORT
%token            SUM AVERAGE MIN MAX
%token            AND OR NOT
%token            GT LT GE LE EQ NE
%token            PLUS MINUS TIMES DIVIDE
%token            ASSIGN SEMI COMMA COLON
%token            LPAREN RPAREN LBRACE RBRACE

/* Precedência */
%left   OR
%left   AND
%left   GT LT GE LE EQ NE
%left   PLUS MINUS
%left   TIMES DIVIDE
%right  NOT UMINUS

/* Não-terminais e tipos */
%type  <stmt_list> start program
%type  <stmt>      statement statement_block
%type  <expr>      expression logical_or logical_and comparison
%type  <expr>      addition_subtraction multiplication_division unary primary
%type  <expr_list> expression_list
%type  <expr>      expression_or_range

%%

/* Símbolo inicial que captura o programa inteiro */
start
    : program              { program_root = $1; }
    ;

/* Programa: lista de statements */
program
    : /* vazio */          { $$ = NULL; }
    | program statement    { $$ = stmt_append($1, $2); }
    ;

/* Statements */
statement
    : CELL ASSIGN expression SEMI
        { $$ = make_assign_stmt($1, $3); }
    | IF expression THEN statement_block
        { $$ = make_if_stmt($2, $4); }
    | WHILE expression statement_block
        { $$ = make_while_stmt($2, $3); }
    | TABLE SEMI
        { $$ = make_table_stmt(); }
    | EXPORT TEXT SEMI
        { $$ = make_export_stmt($2); }
    ;

/* Bloco de statements dentro de IF/WHILE */
statement_block
    : statement
        { $$ = $1; }
    | LBRACE program RBRACE
        { $$ = $2; }
    ;

/* Expressão */
expression
    : logical_or
        { $$ = $1; }
    ;

/* OR lógico */
logical_or
    : logical_and
        { $$ = $1; }
    | logical_or OR logical_and
        { $$ = make_binary_expr(OP_OR, $1, $3); }
    ;

/* AND lógico */
logical_and
    : comparison
        { $$ = $1; }
    | logical_and AND comparison
        { $$ = make_binary_expr(OP_AND, $1, $3); }
    ;

/* Comparações */
comparison
    : addition_subtraction
        { $$ = $1; }
    | comparison GT  addition_subtraction
        { $$ = make_binary_expr(OP_GT, $1, $3); }
    | comparison LT  addition_subtraction
        { $$ = make_binary_expr(OP_LT, $1, $3); }
    | comparison GE  addition_subtraction
        { $$ = make_binary_expr(OP_GE, $1, $3); }
    | comparison LE  addition_subtraction
        { $$ = make_binary_expr(OP_LE, $1, $3); }
    | comparison EQ  addition_subtraction
        { $$ = make_binary_expr(OP_EQ, $1, $3); }
    | comparison NE  addition_subtraction
        { $$ = make_binary_expr(OP_NE, $1, $3); }
    ;

/* + e - */
addition_subtraction
    : multiplication_division
        { $$ = $1; }
    | addition_subtraction PLUS multiplication_division
        { $$ = make_binary_expr(OP_ADD, $1, $3); }
    | addition_subtraction MINUS multiplication_division
        { $$ = make_binary_expr(OP_SUB, $1, $3); }
    ;

/* * e / */
multiplication_division
    : unary
        { $$ = $1; }
    | multiplication_division TIMES unary
        { $$ = make_binary_expr(OP_MUL, $1, $3); }
    | multiplication_division DIVIDE unary
        { $$ = make_binary_expr(OP_DIV, $1, $3); }
    ;

/* Unários */
unary
    : NOT unary
        { $$ = make_unary_expr(OP_NOT, $2); }
    | MINUS unary     %prec UMINUS
        { $$ = make_unary_expr(OP_NEG, $2); }
    | primary
        { $$ = $1; }
    ;

/* Literais, referências e chamadas */
primary
    : INT
        { $$ = make_int_expr($1); }
    | FLOAT
        { $$ = make_float_expr($1); }
    | TEXT
        { $$ = make_text_expr($1); }
    | CELL
        { $$ = make_cell_expr($1); }
    | SUM     LPAREN expression_list RPAREN
        { $$ = make_call_expr("SUM",     $3); }
    | AVERAGE LPAREN expression_list RPAREN
        { $$ = make_call_expr("AVERAGE", $3); }
    | MIN     LPAREN expression_list RPAREN
        { $$ = make_call_expr("MIN",     $3); }
    | MAX     LPAREN expression_list RPAREN
        { $$ = make_call_expr("MAX",     $3); }
    | LPAREN expression RPAREN
        { $$ = $2; }
    ;

/* Lista de expressões (argumentos) */
expression_list
    : expression_or_range
        { $$ = $1; }
    | expression_list COMMA expression_or_range
        { $$ = expr_append($1, $3); }
    ;

/* Expressão ou rango */
expression_or_range
    : expression
        { $$ = $1; }
    | CELL COLON CELL
       {
         $$ = make_range_expr($1, $3);
       }
    ;

%%
