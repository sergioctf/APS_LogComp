// interp.c
// Interpreter para LangCell: avalia AST, expande ranges em chamadas de função,
// executa atribuições, condicionais, loops, TABLE e EXPORT.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

// sum_helper, avg_helper, min_helper, max_helper
double sum_helper(int n, double *vals) {
  double acc = 0.0;
  for (int i = 0; i < n; ++i) acc += vals[i];
  return acc;
}

double avg_helper(int n, double *vals) {
  double acc = sum_helper(n, vals);
  return n>0 ? acc / n : 0.0;
}

double min_helper(int n, double *vals) {
  if (n<=0) return 0.0;
  double m = vals[0];
  for (int i = 1; i < n; ++i) if (vals[i] < m) m = vals[i];
  return m;
}

double max_helper(int n, double *vals) {
  if (n<=0) return 0.0;
  double m = vals[0];
  for (int i = 1; i < n; ++i) if (vals[i] > m) m = vals[i];
  return m;
}


// export_csv
void export_csv(const char *filename,
                int n_cells, const char **names, double *values) {
  FILE *f = fopen(filename, "w");
  for (int i = 0; i < n_cells; ++i)
    fprintf(f, "%s,%g\n", names[i], values[i]);
  fclose(f);
}


// Se string contém vírgula, aspas ou newline, envolve em "..." e duplica as aspas internas
static char *csv_escape(const char *s) {
  int need = 0;
  for (const char *p = s; *p; ++p) {
      if (*p == '"' || *p == ',' || *p == '\n') need++;
  }
  // sem escapes, retorna copia direta
  if (need == 0) return strdup(s);

  size_t len = strlen(s) + need + 2 + 1;
  char *out = malloc(len);
  char *o = out;
  *o++ = '"';
  for (const char *p = s; *p; ++p) {
      if (*p == '"') {
          *o++ = '"';
          *o++ = '"';
      } else {
          *o++ = *p;
      }
  }
  *o++ = '"';
  *o   = '\0';
  return out;
}


// Tipo genérico de valor
typedef enum { V_INT, V_FLOAT, V_TEXT } ValueKind;
typedef struct {
    ValueKind kind;
    union {
        int    ival;
        double fval;
        char  *sval;
    };
} Value;

// Estrutura para o mapa de células
typedef struct Cell {
    char *name;
    Value val;
    struct Cell *next;
} Cell;

static Cell *cell_map = NULL;

// Insere ou atualiza valor de célula, mantendo ordem de inserção
static void map_set(const char *name, Value v) {
    for (Cell *c = cell_map; c; c = c->next) {
        if (strcmp(c->name, name) == 0) {
            c->val = v;
            return;
        }
    }
    // inserir no final da lista para preservar ordem
    Cell *n = malloc(sizeof *n);
    n->name = strdup(name);
    n->val  = v;
    n->next = NULL;
    if (!cell_map) {
        cell_map = n;
    } else {
        Cell *it = cell_map;
        while (it->next) it = it->next;
        it->next = n;
    }
}

// Recupera valor de célula (0 se não existir)
static Value map_get(const char *name) {
    for (Cell *c = cell_map; c; c = c->next) {
        if (strcmp(c->name, name) == 0)
            return c->val;
    }
    return (Value){.kind = V_INT, .ival = 0};
}

// Avalia uma expressão e retorna um Value
static Value eval_expr(Expr *e) {
    switch (e->kind) {
      case EXPR_INT:
        return (Value){.kind=V_INT,   .ival=e->ival};
      case EXPR_FLOAT:
        return (Value){.kind=V_FLOAT, .fval=e->fval};
      case EXPR_TEXT:
        return (Value){.kind=V_TEXT,  .sval=e->sval};
      case EXPR_CELL:
        return map_get(e->sval);
      case EXPR_UNARY: {
        Value sub = eval_expr(e->un.sub);
        if (e->un.op == OP_NEG) {
            if (sub.kind == V_FLOAT) sub.fval = -sub.fval;
            else                      sub.ival = -sub.ival;
        } else { // OP_NOT
            int cond = (sub.kind == V_FLOAT ? (int)sub.fval : sub.ival);
            sub.kind = V_INT;
            sub.ival = !cond;
        }
        return sub;
      }
      case EXPR_BINARY: {
        Value L = eval_expr(e->bin.left);
        Value R = eval_expr(e->bin.right);
        double l = (L.kind == V_FLOAT ? L.fval : L.ival);
        double r = (R.kind == V_FLOAT ? R.fval : R.ival);
        switch (e->bin.op) {
          case OP_ADD: return (Value){.kind=V_FLOAT, .fval = l + r};
          case OP_SUB: return (Value){.kind=V_FLOAT, .fval = l - r};
          case OP_MUL: return (Value){.kind=V_FLOAT, .fval = l * r};
          case OP_DIV: return (Value){.kind=V_FLOAT, .fval = l / r};
          case OP_GT:  return (Value){.kind=V_INT,   .ival = l > r};
          case OP_LT:  return (Value){.kind=V_INT,   .ival = l < r};
          case OP_GE:  return (Value){.kind=V_INT,   .ival = l >= r};
          case OP_LE:  return (Value){.kind=V_INT,   .ival = l <= r};
          case OP_EQ:  return (Value){.kind=V_INT,   .ival = l == r};
          case OP_NE:  return (Value){.kind=V_INT,   .ival = l != r};
          case OP_AND: return (Value){.kind=V_INT,   .ival = (int)l && (int)r};
          case OP_OR:  return (Value){.kind=V_INT,   .ival = (int)l || (int)r};
        }
        break;
      }
      case EXPR_CALL: {
        const char *fn = e->call.fname;
        double *vals = NULL;
        size_t  n    = 0;

        // Expande args: ranges e expressões
        for (Expr *arg = e->call.args; arg; arg = arg->next) {
            if (arg->kind == EXPR_RANGE) {
                char sc = arg->range.start_cell[0];
                char ec = arg->range.end_cell[0];
                int  sr = atoi(arg->range.start_cell + 1);
                int  er = atoi(arg->range.end_cell   + 1);
                for (char c = sc; c <= ec; ++c) {
                    for (int r = sr; r <= er; ++r) {
                        char cell[16];
                        snprintf(cell, sizeof cell, "%c%d", c, r);
                        Value v = map_get(cell);
                        double x = (v.kind == V_FLOAT ? v.fval : v.ival);
                        vals = realloc(vals, (n+1)*sizeof *vals);
                        vals[n++] = x;
                    }
                }
            } else {
                Value v = eval_expr(arg);
                double x = (v.kind == V_FLOAT ? v.fval : v.ival);
                vals = realloc(vals, (n+1)*sizeof *vals);
                vals[n++] = x;
            }
        }

        if (n == 0) {
            fprintf(stderr, "Erro: chamada %s sem argumentos\n", fn);
            return (Value){.kind=V_INT, .ival = 0};
        }

        if (strcmp(fn, "SUM") == 0) {
            double acc = 0;
            for (size_t i = 0; i < n; ++i) acc += vals[i];
            free(vals);
            return (Value){.kind=V_FLOAT, .fval = acc};
        }
        if (strcmp(fn, "AVERAGE") == 0) {
            double acc = 0;
            for (size_t i = 0; i < n; ++i) acc += vals[i];
            free(vals);
            return (Value){.kind=V_FLOAT, .fval = acc / n};
        }
        if (strcmp(fn, "MIN") == 0) {
            double m = vals[0];
            for (size_t i = 1; i < n; ++i) if (vals[i] < m) m = vals[i];
            free(vals);
            return (Value){.kind=V_FLOAT, .fval = m};
        }
        if (strcmp(fn, "MAX") == 0) {
            double m = vals[0];
            for (size_t i = 1; i < n; ++i) if (vals[i] > m) m = vals[i];
            free(vals);
            return (Value){.kind=V_FLOAT, .fval = m};
        }

        fprintf(stderr, "Erro: função desconhecida %s\n", fn);
        free(vals);
        return (Value){.kind=V_INT, .ival = 0};
      }
      case EXPR_RANGE:
        // isolado, retorna 0
        return (Value){.kind=V_INT, .ival = 0};
    }
    // fallback
    return (Value){.kind=V_INT, .ival = 0};
}

// Executa uma lista de statements
static int interpret_stmt(Stmt *s) {
    while (s) {
        switch (s->kind) {
          case STMT_ASSIGN: {
            Value v = eval_expr(s->assign.expr);
            map_set(s->assign.cell, v);
            break;
          }
          case STMT_IF: {
            Value c = eval_expr(s->ifs.cond);
            int cond = (c.kind == V_FLOAT ? (int)c.fval : c.ival);
            if (cond) interpret_stmt(s->ifs.then_branch);
            break;
          }
          case STMT_WHILE: {
            Value c = eval_expr(s->whiles.cond);
            int cond = (c.kind == V_FLOAT ? (int)c.fval : c.ival);
            while (cond) {
                interpret_stmt(s->whiles.body);
                c = eval_expr(s->whiles.cond);
                cond = (c.kind == V_FLOAT ? (int)c.fval : c.ival);
            }
            break;
          }
          case STMT_TABLE: {
            for (Cell *c = cell_map; c; c = c->next) {
                if (c->val.kind == V_FLOAT)
                    printf("%s\t%g\n", c->name, c->val.fval);
                else if (c->val.kind == V_INT)
                    printf("%s\t%d\n", c->name, c->val.ival);
                else
                    printf("%s\t%s\n", c->name, c->val.sval);
            }
            break;
          }
          case STMT_EXPORT: {
            FILE *f = fopen(s->exp.filename, "w");
            if (!f) { perror("fopen"); return 1; }
            for (Cell *c = cell_map; c; c = c->next) {
              if (c->val.kind == V_FLOAT) {
                fprintf(f, "%s,%g\n", c->name, c->val.fval);
              } else if (c->val.kind == V_INT) {
                fprintf(f, "%s,%d\n", c->name, c->val.ival);
              } else {
                char *esc = csv_escape(c->val.sval);
                fprintf(f, "%s,%s\n", c->name, esc);
                free(esc);
              }
            }
            fclose(f);
            break;
          }
        }
        s = s->next;
    }
    return 0;
}

int interpret(Stmt *program) {
    return interpret_stmt(program);
}
