#include <stdio.h>
#include <string.h>
#include "sema.h"
#include "ast.h"

// Helper: promove INT -> FLOAT quando necessário
static Type promote(Type a, Type b) {
    if (a == TYPE_ERROR || b == TYPE_ERROR) return TYPE_ERROR;
    if (a == TYPE_TEXT  || b == TYPE_TEXT)  return TYPE_ERROR;
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

Type analyze_expr(Expr *e) {
    if (!e) return TYPE_ERROR;
    switch (e->kind) {
      case EXPR_INT:
        return TYPE_INT;
      case EXPR_FLOAT:
        return TYPE_FLOAT;
      case EXPR_TEXT:
        return TYPE_TEXT;
      case EXPR_CELL:
        // células podem conter qualquer tipo, tratamos como numérico
        return TYPE_FLOAT;
      case EXPR_UNARY: {
        Type t = analyze_expr(e->un.sub);
        if (e->un.op == OP_NOT) {
            if (t == TYPE_TEXT) {
                fprintf(stderr, "Erro semântico: NOT só para numéricos\n");
                return TYPE_ERROR;
            }
            return TYPE_INT;
        } else { // OP_NEG
            if (t == TYPE_TEXT) {
                fprintf(stderr, "Erro semântico: negação só para numéricos\n");
                return TYPE_ERROR;
            }
            return t;
        }
      }
      case EXPR_BINARY: {
        Type l = analyze_expr(e->bin.left);
        Type r = analyze_expr(e->bin.right);
        switch (e->bin.op) {
          case OP_ADD: case OP_SUB:
          case OP_MUL: case OP_DIV:
            if (l == TYPE_TEXT || r == TYPE_TEXT) {
                fprintf(stderr, "Erro semântico: operação aritmética só para numéricos\n");
                return TYPE_ERROR;
            }
            return promote(l, r);
          case OP_GT: case OP_LT:
          case OP_GE: case OP_LE:
          case OP_EQ: case OP_NE:
            if (l == TYPE_TEXT || r == TYPE_TEXT) {
                fprintf(stderr, "Erro semântico: comparação só para numéricos\n");
                return TYPE_ERROR;
            }
            return TYPE_INT;
          case OP_AND: case OP_OR:
            if (l == TYPE_TEXT || r == TYPE_TEXT) {
                fprintf(stderr, "Erro semântico: lógica só para numéricos\n");
                return TYPE_ERROR;
            }
            return TYPE_INT;
        }
        break;
      }
      case EXPR_CALL: {
        // percorre args, aceitando ranges dentro de calls
        Expr *arg = e->call.args;
        int cnt = 0;
        Type acc = TYPE_ERROR;
        while (arg) {
            Type t;
            if (arg->kind == EXPR_RANGE) {
                // range em call é válido: tratá-lo como numérico
                t = TYPE_FLOAT;
            } else {
                t = analyze_expr(arg);
            }
            if (t == TYPE_ERROR) return TYPE_ERROR;
            acc = (cnt == 0 ? t : promote(acc, t));
            arg = arg->next;
            cnt++;
        }
        if (cnt == 0) {
            fprintf(stderr, "Erro semântico: chamada %s sem argumentos\n",
                    e->call.fname);
            return TYPE_ERROR;
        }
        // SUM, MIN, MAX retornam numérico; AVERAGE sempre float
        if (strcmp(e->call.fname, "AVERAGE") == 0)
            return TYPE_FLOAT;
        if (strcmp(e->call.fname, "SUM") == 0 ||
            strcmp(e->call.fname, "MIN") == 0 ||
            strcmp(e->call.fname, "MAX") == 0)
            return acc;
        fprintf(stderr, "Erro semântico: função desconhecida %s\n",
                e->call.fname);
        return TYPE_ERROR;
      }
      case EXPR_RANGE:
        // erro se range isolado
        fprintf(stderr, "Erro semântico: range não dentro de função\n");
        return TYPE_ERROR;
    }
    return TYPE_ERROR;
}

int analyze_stmt_list(Stmt *s) {
    int errs = 0;
    for (; s; s = s->next) {
        switch (s->kind) {
          case STMT_ASSIGN: {
            Type t = analyze_expr(s->assign.expr);
            if (t == TYPE_ERROR) errs++;
            break;
          }
          case STMT_IF: {
            Type t = analyze_expr(s->ifs.cond);
            if (t == TYPE_ERROR || t == TYPE_TEXT) {
                fprintf(stderr,
                        "Erro semântico: condição IF deve ser numérica\n");
                errs++;
            }
            errs += analyze_stmt_list(s->ifs.then_branch);
            break;
          }
          case STMT_WHILE: {
            Type t = analyze_expr(s->whiles.cond);
            if (t == TYPE_ERROR || t == TYPE_TEXT) {
                fprintf(stderr,
                        "Erro semântico: condição WHILE deve ser numérica\n");
                errs++;
            }
            errs += analyze_stmt_list(s->whiles.body);
            break;
          }
          case STMT_TABLE:
          case STMT_EXPORT:
            // sem verificações adicionais
            break;
        }
    }
    return errs;
}
