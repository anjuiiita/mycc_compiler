
all: developers.pdf mycc

SOURCES= mycc.cc lexer.cc parsehelp.cc
HEADERS= lexer.h parsehelp.h
GENERATED= tokens.cc grammar.tab.h grammar.tab.c grammar.tab.cc
OBJECTS= mycc.o lexer.o tokens.o grammar.tab.o parsehelp.o
TARFILES= $(SOURCES) $(HEADERS) Makefile tokens.ll grammar.y developers.tex
DIR=$(notdir $(realpath .))


clean:
	rm developers.pdf developers.aux developers.log mycc $(OBJECTS) $(GENERATED)

depend:
	makedepend -DSKIP_SYSTEM_INCLUDES $(SOURCES) $(GENERATED)

tarball: bare3.tar.gz

bare3.tar.gz: $(TARFILES) 
	tar czf $@ --cd .. $(addprefix $(DIR)/,$(TARFILES))


# Special rules:

developers.pdf: developers.tex
	pdflatex developers.tex

mycc: $(OBJECTS)
	g++ -o mycc $(OBJECTS)

tokens.cc: tokens.ll
	flex -o tokens.cc tokens.ll

grammar.tab.h grammar.tab.c: grammar.y
	bison -d grammar.y
	#bison --debug --verbose -d grammar.y

grammar.tab.cc: grammar.tab.c
	sed 's/grammar.tab.c/grammar.tab.cc/' grammar.tab.c > grammar.tab.cc

grammar.tab.o: grammar.tab.cc
	g++ -c grammar.tab.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.

mycc.o: lexer.h parsehelp.h
lexer.o: lexer.h parsehelp.h grammar.tab.h
tokens.o: lexer.h parsehelp.h grammar.tab.h
grammar.tab.o: lexer.h parsehelp.h grammar.tab.h
grammar.tab.o: lexer.h parsehelp.h grammar.tab.h
