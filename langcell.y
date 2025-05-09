%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Protótipos */
int  yylex(void);
int  yyerror(const char *s);
%}

/* Tipos semânticos */
%union {
    int     ival;
    double  fval;
    char   *sval;
}

/* Tokens com tipo */
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

/* Precedência e associatividade */
%left   OR
%left   AND
%left   GT LT GE LE EQ NE
%left   PLUS MINUS
%left   TIMES DIVIDE
%right  NOT UMINUS

%%

program:
      /* vazio */
    | program statement
    ;

statement:
      CELL ASSIGN expression SEMI
    | IF    expression THEN statement_block
    | WHILE expression         statement_block
    | TABLE SEMI
    | EXPORT TEXT   SEMI
    ;

statement_block:
      statement
    | LBRACE program RBRACE
    ;

expression:
    logical_or
    ;

logical_or:
      logical_and
    | logical_or OR logical_and
    ;

logical_and:
      comparison
    | logical_and AND comparison
    ;

comparison:
      addition_subtraction
    | comparison GT  addition_subtraction
    | comparison LT  addition_subtraction
    | comparison GE  addition_subtraction
    | comparison LE  addition_subtraction
    | comparison EQ  addition_subtraction
    | comparison NE  addition_subtraction
    ;

addition_subtraction:
      multiplication_division
    | addition_subtraction PLUS  multiplication_division
    | addition_subtraction MINUS multiplication_division
    ;

multiplication_division:
      unary
    | multiplication_division TIMES unary
    | multiplication_division DIVIDE unary
    ;

unary:
      NOT   unary
    | MINUS unary         %prec UMINUS
    | primary
    ;

primary:
      INT
    | FLOAT
    | TEXT
    | CELL
    | SUM     LPAREN expression_list RPAREN
    | AVERAGE LPAREN expression_list RPAREN
    | MIN     LPAREN expression_list RPAREN
    | MAX     LPAREN expression_list RPAREN
    | LPAREN  expression RPAREN
    ;

expression_list:
      expression_or_range
    | expression_list COMMA expression_or_range
    ;

expression_or_range:
      expression
    | CELL COLON CELL
    ;

%%

int yyerror(const char *s) {
    fprintf(stderr, "Erro de sintaxe: %s\n", s);
    return 1;
}

int main(void) {
    return yyparse();
}
