# Ferramentas
LEX       := flex
YACC      := bison
CC        := gcc
CFLAGS    := -Wall -Wextra -g
LDFLAGS   := -lfl

# Alvos “phony”
.PHONY: all clean

# Alvo padrão
all: langcell

# Link do executável
langcell: langcell.tab.o langcell.lex.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Geração do parser (Bison)
langcell.tab.c langcell.tab.h: langcell.y
	$(YACC) -d -o langcell.tab.c langcell.y

# Geração do scanner (Flex)
langcell.lex.c: langcell.l langcell.tab.h
	$(LEX) -o langcell.lex.c langcell.l

# Compilações individuais
langcell.tab.o: langcell.tab.c
	$(CC) $(CFLAGS) -c $< -o $@

langcell.lex.o: langcell.lex.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f langcell.tab.c langcell.tab.h langcell.lex.c \
	      langcell.tab.o langcell.lex.o langcell
