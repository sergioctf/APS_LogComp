// main.cpp

#include <cstdio>
#include "ast.h"
#include "sema.h"
#include "interp.h"
#include "codegen.h"

extern "C" int yyparse(void);
extern "C" int yyerror(const char *s);

// O parser define e preenche esta variável
Stmt *program_root = nullptr;

int yyerror(const char *s) {
    std::fprintf(stderr, "Erro de sintaxe: %s\n", s);
    return 1;
}

int main(int argc, char **argv) {
    // 1) Parsing
    if (yyparse() != 0) {
        return 1;
    }

    // 2) Análise semântica
    int sem_errs = analyze_stmt_list(program_root);
    if (sem_errs > 0) {
        std::fprintf(stderr,
                     "Foram encontrados %d erro(s) semânticos.\n",
                     sem_errs);
        return 1;
    }

    // 3) Geração de código e JIT (via LLVM)
    init_llvm("LangCellModule");
    generate_code(program_root);
    return run_code();

    // Se você ainda quiser usar o interpretador em vez do JIT,
    // basta comentar as três linhas acima e descomentar esta:
    // return interpret(program_root);
}
