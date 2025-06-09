// main.cpp

#include <cstdio>
#include "ast.h"
#include "sema.h"
#include "interp.h"   // (se você criou este cabeçalho)

extern "C" int yyparse(void);
extern "C" int yyerror(const char *s);

Stmt *program_root = nullptr;  // única definição

int yyerror(const char *s) {
    std::fprintf(stderr, "Erro de sintaxe: %s\n", s);
    return 1;
}

int main() {
    // 1) parsing
    if (yyparse() != 0) {
        return 1;
    }

    // 2) semântica
    int sem_errs = analyze_stmt_list(program_root);
    if (sem_errs > 0) {
        std::fprintf(stderr, "Foram encontrados %d erro(s) semânticos.\n", sem_errs);
        return 1;
    }

    // 3) interpretação
    return interpret(program_root);
}
