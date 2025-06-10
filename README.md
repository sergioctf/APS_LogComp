# APS de Lógica Computacional 2025.1 — Insper  
**Sérgio Carmelo Tôrres Filho — Engenharia da Computação**

---

# LangCell

**LangCell** é uma mini-linguagem inspirada em planilhas da plataforma Excel, onde cada variável é uma “célula” (A1, B2, …). A execução é feita por um JIT baseado em LLVM, gerando IR otimizado em memória. Assim o usuário pode escrever expressões aritméticas, lógicas e condicionais, além de usar funções de agregação como `SUM`, `AVERAGE`, etc. e exportar os resultados para um arquivo CSV ou imprimir uma tabela no console.
## Objetivos
1. **Criar uma linguagem de planilha** com sintaxe simples e intuitiva, permitindo manipulação de dados numéricos e textuais.
2. **Implementar análise léxica e sintática** usando Flex e Bison, gerando um AST (Árvore de Sintaxe Abstrata).
3. **Desenvolver um JIT (Just-In-Time Compiler)** com LLVM, capaz de executar expressões e gerar código otimizado em tempo real.

---

## Recursos Implementados

1. **Tipos de valores**  
   - `double`: suporta literais inteiros e de ponto flutuante  
   - `text`: literais entre aspas (e.g. `"olá"`)

2. **Operadores Aritméticos**  
    ```lc
    +, -, *, /  
    // ex: A1 = 2.0 + 3.0; // A1 recebe 5.0
    ```

3. **Comparadores (double → boolean→double)**

   ```lc
   >, <, >=, <=, ==, !=  
   // ex: (A1 > A2) retorna 1.0 ou 0.0
   ```

4. **Operadores Lógicos**

   ```lc
   AND, OR, NOT  
   // curinga: avaliam 0.0 como false, !=0 como true, resultado em 1.0/0.0
   ```

5. **Condicionais**

   ```lc
   IF <expr> THEN <statement>  
   // bloco ou única instrução
   ```

6. **Laços**

   ```lc
   WHILE <expr> { … }  
   ```

7. **Funções de planilha** (agregações sobre ranges e expressões)

   ```lc
   SUM(range, [args…])       // soma valores
   AVERAGE(range, [args…])   // média
   MIN(range, [args…])       // mínimo
   MAX(range, [args…])       // máximo
   ```

   * Suporta ranges como `A1:C3`
   * Suporta chamada aninhada: `SUM(A1:C3, SUM(A1:B2))`

8. **Expressões aninhadas**

   * combinação arbitrária de aritmética, comparações e lógicos
   * exemplo: `C1 = (A1 + A2) * 2 - A3 / 5`

9. **Impressão Tabular**

   ```lc
   TABLE;
   ```

   * Primeiro imprime todas as células numéricas
   * Depois todas as de texto

10. **Exportação para CSV**

    * Gerado no IR: `fopen`/`fprintf`/`fclose`
    * Formatos `%s,%g\n` (números) e `%s,%s\n` (textos)

    ```lc
    EXPORT "saida.csv";
    ```

---

## Gramática (EBNF resumida)

```ebnf
<program>        ::= { <statement> }

<statement>      ::= <cell> "=" <expr> ";" 
                 | "IF" <expr> "THEN" <block>
                 | "WHILE" <expr> <block>
                 | "TABLE" ";"
                 | "EXPORT" <text> ";"

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
                 | <text>
                 | "(" <expr> ")"

<cell>           ::= [A–Z]+ [0–9]+
<number>         ::= integer | float
<text>           ::= '"' .* '"'
```

---

## Exemplo Completo

```lc
A1 = 10;
A2 = 20;
A3 = 30;
// soma simples
S1 = SUM(A1:A3);                // 60
// soma aninhada
S2 = SUM(A1:A3, S1);            // 60 + 60 = 120
// média mista
AVG2 = AVERAGE(S2, 40, 50);      // (120 + 40 + 50) / 3 = 70
// expressão aritmética 
C1 = A1 * 2 + A2 / 5 - A3;       // 10*2 + 20/5 -30 = 20 +4 -30 = -6

TABLE;
EXPORT "saida.csv";
```

---

## Status das Tarefas

| Tarefa | Descrição                                              | Status      |
| ------ | ------------------------------------------------------ | ----------- |
| 1      | Gramática (EBNF)                                       | ✅ Concluída |
| 2      | Análise Léxica (Flex) e Sintática (Bison)              | ✅ Concluída |
| 3      | Semântica + geração de IR/execução via LLVM MCJIT      | ✅ Concluída |
| 4      | Apresentação da Linguagem (README)            | ✅ Concluída |
| X      | Suporte a comparadores e lógicos em IR                 | ✅ Concluída |
| X      | Literais de texto e tabela completa                    | ✅ Concluída |
| X      | Exportação CSV    | ✅ Concluída |
| X      | 5 casos de teste cobrindo todas as combinações | ✅ Concluída |

**Entrega final**: 10/Jun/2025

---

## Como Usar

1. **Compilar**

   ```bash
   make
   ```

2. **Executar um script**

   ```bash
   ./langcell < arquivo.lc
   ```

3. **Ver saída em tabela** (no console se usar `TABLE;`)
   Ex.:
   ```
   A1    11
   B1    12
   ...
   ```

4. **Ver CSV gerado** (se `EXPORT` usado)

   ```bash
   cat saida.csv
   ```


---

## Testes

  Foram criados 5 casos de teste cobrindo todas as combinações de operadores, funções e estruturas de controle. Eles estão localizados no projeto e são chamados `test1.lc`, `test2.lc`, etc. Cada teste verifica a execução correta de expressões, agregações e controle de fluxo.

---

*Projeto desenvolvido por Sérgio Carmelo Tôrres Filho*
