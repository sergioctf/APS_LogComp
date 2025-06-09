%{
#include "ast.h"
#include "langcell.tab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern YYSTYPE yylval;
%}

%option noyywrap nounput noinput

%%

"//"[^\n]*              { /* ignora comentário de linha */ }
"/*"(.|\n)*"*/"         { /* ignora comentário de bloco */ }
[ \t\r\n]+              { /* ignora espaços em branco */ }

"IF"                    { return IF; }
"THEN"                  { return THEN; }
"WHILE"                 { return WHILE; }
"TABLE"                 { return TABLE; }
"EXPORT"                { return EXPORT; }

"SUM"                   { return SUM; }
"AVERAGE"               { return AVERAGE; }
"MIN"                   { return MIN; }
"MAX"                   { return MAX; }

"AND"                   { return AND; }
"OR"                    { return OR; }
"NOT"                   { return NOT; }

">="                    { return GE; }
"<="                    { return LE; }
"=="                    { return EQ; }
"!="                    { return NE; }
">"                     { return GT; }
"<"                     { return LT; }

"="                     { return ASSIGN; }
";"                     { return SEMI; }
","                     { return COMMA; }
":"                     { return COLON; }
"("                     { return LPAREN; }
")"                     { return RPAREN; }
"{"                     { return LBRACE; }
"}"                     { return RBRACE; }
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return TIMES; }
"/"                     { return DIVIDE; }

[0-9]+\.[0-9]+          {
                          yylval.fval = atof(yytext);
                          return FLOAT;
                        }

[0-9]+                  {
                          yylval.ival = atoi(yytext);
                          return INT;
                        }

\"[^\"]*\"              {
                          int len = yyleng - 2;
                          char *s = malloc(len + 1);
                          if (!s) exit(1);
                          memcpy(s, yytext + 1, len);
                          s[len] = '\0';
                          yylval.sval = s;
                          return TEXT;
                        }

[A-Z][0-9]+             {
                          yylval.sval = strdup(yytext);
                          return CELL;
                        }

.                       {
                          fprintf(stderr, "Unexpected char: %s\n", yytext);
                          exit(1);
                        }

%%
