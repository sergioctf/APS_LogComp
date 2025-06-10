# Ferramentas
LEX           := flex
YACC          := bison
CC            := gcc
CXX           := g++
CFLAGS        := -Wall -Wextra -g
LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
LLVM_LDFLAGS  := $(shell llvm-config --libs core mcjit native) -ldl -lpthread

CXXFLAGS := $(CFLAGS) $(LLVM_CXXFLAGS)
LDFLAGS  := -lfl $(LLVM_LDFLAGS)

# Objetos
OBJS := main.o         \
        sema.o         \
        langcell.tab.o \
        langcell.lex.o \
        ast.o          \
        interp.o       \
        codegen.o

.PHONY: all clean
all: langcell

langcell: $(OBJS)
	$(CXX) $(CXXFLAGS) -rdynamic $^ -o $@ $(LDFLAGS)

# Geração do parser
langcell.tab.c langcell.tab.h: langcell.y
	$(YACC) -d -o langcell.tab.c langcell.y

# Geração do lexer
langcell.lex.c: langcell.l langcell.tab.h ast.h interp.h sema.h
	$(LEX) -o langcell.lex.c langcell.l

# Compilação dos objetos
langcell.tab.o: langcell.tab.c
	$(CC) $(CFLAGS) -c $< -o $@

langcell.lex.o: langcell.lex.c
	$(CC) $(CFLAGS) -c $< -o $@

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c $< -o $@

interp.o: interp.c ast.h interp.h
	$(CC) $(CFLAGS) -c $< -o $@

sema.o: sema.c sema.h ast.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.cpp ast.h sema.h codegen.h interp.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

codegen.o: codegen.cpp codegen.h ast.h interp.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o langcell \
	       langcell.tab.c langcell.tab.h langcell.lex.c
