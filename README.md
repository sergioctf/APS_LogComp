# APS de Lógica Computacional 2025.1 — Insper  
## Sérgio Carmelo Tôrres Filho — Engenharia da Computação  

---

# LangCell

**LangCell** é uma mini-linguagem de programação inspirada em planilhas, em que cada variável é uma “célula” (A1, B2, …) e é possível usar expressões, condicionais, loops e imprimir tudo em tabela.

---

## Recursos Implementados

- **Variáveis** (células) do tipo `double`:  
  - Inteiros (`123`)  
  - Float (`3.14`)  
- **Operadores Aritméticos**: `+`, `-`, `*`, `/`  
- **Condicionais**:  
```

IF <expr> THEN <statement-block>

```
- **Laços**:  
```

WHILE <expr> { … }

```
- **Impressão em formato tabular**:  
```

TABLE;

````

---

## Gramática (EBNF resumida)

```ebnf
<program>        ::= { <statement> }

<statement>      ::= <assign> ";" 
                 | "IF" <expr> "THEN" <block>
                 | "WHILE" <expr> <block>
                 | "TABLE" ";"

<assign>         ::= <cell> "=" <expr>

<block>          ::= <statement>
                 | "{" { <statement> } "}"

<expr>           ::= <expr> "+" <term> 
                 | <expr> "-" <term> 
                 | <term>

<term>           ::= <term> "*" <factor> 
                 | <term> "/" <factor> 
                 | <factor>

<factor>         ::= <number> 
                 | <cell> 
                 | "(" <expr> ")"

<cell>           ::= [A–Z]+ [0–9]+
<number>         ::= integer | float
````

---

## Exemplos

```lc
// 1) Atribuições e operações
A1 = 1;
A2 = 2;
B1 = A1 + A2;

// 2) Condicional
IF A1 - A2 THEN {
  C1 = A1 * A2;
}

// 3) Loop
WHILE A1 < 5 {
  A1 = A1 + 1;
}

// 4) Imprime tudo em tabela
TABLE;
```

Saída esperada:

```
A1    5
A2    2
B1    3
C1    2
```

---

## Status das Tarefas

| Tarefa | Descrição                                                       | Status      |
| ------ | --------------------------------------------------------------- | ----------- |
| 1      | Gramática EBNF                                                  | ✅ Concluída |
| 2      | Análise Léxica (Flex) e Sintática (Bison)                       | ✅ Concluída |
| 3      | Semântica + geração de código/execução com LLVM MCJIT           | ✅ Concluída |
| 4      | Testes de exemplo (demo2.lc, test\_suite.lc, validos/invalidos) | ✅ Concluída |
| 5      | Apresentação (slides com motivação, características e exemplos) | ⏳ Pendente  |

**Entrega final**: 10/Jun/2025

---

## Como usar

1. **Build**

   ```bash
   make
   ```
2. **Testar um programa**

   ```bash
   ./langcell < demo2.lc
   ```
3. **Adicionar novos casos de teste**

   * coloque em `validos/` ou `invalidos/` e ajuste `test_suite.lc`
