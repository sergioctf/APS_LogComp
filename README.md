
# APS de Lógica Computacional 2025.1 — Insper  
## Sérgio Carmelo Tôrres Filho — Engenharia da Computação  

---

# LangCell

**LangCell** é uma linguagem de programação inspirada no Excel, onde toda variável é uma célula (A1, B2, …) e fórmulas, loops e condicionais se comportam exatamente como em uma planilha “viva”.

---

## Principais Recursos

- **Células como variáveis**: `A1`, `B2`, …  
- **Valores**:  
  - Inteiros (`123`)  
  - **Floats** (`3.14`)  
  - **Textos** entre aspas (`"Hello"`)  
- **Operadores Aritméticos**: `+`, `-`, `*`, `/`  
- **Funções de Planilha**: `SUM()`, `AVERAGE()`, `MIN()`, `MAX()`  
- **Ranges**: `SUM(A1:A10)`, `AVERAGE(B2:D2)`  
- **Comparadores**: `>`, `<`, `>=`, `<=`, `==`, `!=`  
- **Operadores Lógicos**: `NOT`, `AND`, `OR`  
- **Comentários**:  
  - Linha única: `// comentário`  
  - Bloco: `/* comentário */`  
- **Condicionais**: `IF <expr> THEN <statement> …`  
- **Laços**: `WHILE <expr> { … }`  
- **PRINT de Planilha**: `TABLE;` → Imprime no terminal em formato tabular  
- **Exportação CSV**: `EXPORT "arquivo.csv";` → Salva o estado atual em CSV  

---

## Gramática (EBNF)

```ebnf
<program> ::= { <comment> | <statement> }

<statement> ::=
      <assignment-statement>
    | <if-statement>
    | <while-statement>
    | <table-statement>
    | <export-statement>

<assignment-statement> ::= <cell> "=" <expression> ";"

<if-statement>       ::= "IF" <expression> "THEN" <statement-block>
<while-statement>    ::= "WHILE" <expression> <statement-block>
<table-statement>    ::= "TABLE" ";"
<export-statement>   ::= "EXPORT" <text> ";"

<statement-block> ::= <statement>
                   | "{" { <statement> } "}"

<comment> ::=
      "//" { <character_except_newline> } "\n"
    | "/*" { <character> } "*/"

<expression>      ::= <logical-or>
<logical-or>      ::= <logical-and> { "OR" <logical-and> }
<logical-and>     ::= <comparison>  { "AND" <comparison> }
<comparison>      ::= <addition-subtraction> { <comparator> <addition-subtraction> }
<addition-subtraction> ::= <multiplication-division> { ("+" | "-") <multiplication-division> }
<multiplication-division> ::= <factor> { ("*" | "/") <factor> }

<factor> ::=
      <number>
    | <text>
    | <cell>
    | <function-call>
    | "(" <expression> ")"

<function-call>    ::= <function-name> "(" <expression-list> ")"
<function-name>    ::= "SUM" | "AVERAGE" | "MIN" | "MAX"

<expression-list>  ::= <expression-or-range> { "," <expression-or-range> }
<expression-or-range> ::= <expression>
                       | <cell> ":" <cell>

<cell>   ::= <letter> <number>
<letter> ::= "A" | "B" | … | "Z"

<number> ::= <integer> | <float>
<integer>::= <digit> { <digit> }
<float>  ::= <digit> { <digit> } "." <digit> { <digit> }
<digit>  ::= "0" | "1" | … | "9"

<text>   ::= '"' { <character> } '"'
<character> ::= qualquer caractere imprimível exceto '"'

<comparator> ::= ">" | "<" | ">=" | "<=" | "==" | "!="
````

---

## Exemplos de Uso

```langcell
// Atribuições simples e floats
A1 = 3.14;
B1 = 2;
C1 = A1 * B1;

// Ranges e funções
D1 = SUM(A1:A3, 10);
E1 = AVERAGE(B1:B3);

// Comparações e lógicos
IF D1 >= 20 AND NOT (B1 == 0) THEN {
  F1 = D1 / B1;
}

// Loop
WHILE A1 < 10 {
  A1 = A1 + 0.5;
}

// Imprime tabela formatada
TABLE;

// Exporta para CSV
EXPORT "planilha.csv";
```

---

## Status das Entregas

| Tarefa       | Descrição                                                  | Status                                                                                        |
| ------------ | ---------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| **Tarefa 1** | Definição da proposta, gramática EBNF e exemplos de uso    | ✅ Concluída                                                                                   |
| **Tarefa 2** | Implementação de análise léxica (Flex) e sintática (Bison) | ✅ Concluída — compilador aceita gramática e retorna código 0 em casos válidos, 1 em inválidos |

> **Próxima etapa**: Tarefa 3 — Análise semântica (construção de AST, checagem de tipos e avaliação de expressões).

---

**Entrega final**: 10/Jun/2025

