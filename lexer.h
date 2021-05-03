
#ifndef LEXER_H
#define LEXER_H

#include <iostream>

/*
  Lexer front-end functions.
*/

/*
  Initialize the lexer with the given input file.
    @param  infile        Input file name to use
    @param  tokens_only   If true, just split into tokens (mode 1).
  Return true on success, 0 on failure (can't open file)
*/
int initLexer(const char* infile, char tokens_only, char last_mode, char second_last_mode);

/*
  Display the current 'location' in input:
  filename, linenumber, and text.
*/
void printLocation(std::ostream &out);

/*
  Start an error message.
*/
void startError();

/*
  Start an error message at a particular line number.
*/
void startError(int lineno);

/*
  Print an unclosed comment error message.
    @param start: line number where comment started
    @return 0
*/
int unclosedComment(int start);

/*
  Print a warning about ignored directives
*/
void ignoringDirective(const char* dir);

/*
  Print an invalid token message.
*/
void badToken(const char* x);

/*
  Generated by flex
*/
int yylex();

/*
  Needed by flex
*/
int yywrap();

#define YY_SKIP_YYWRAP

const char* getTokenName(int tok);

/*
  Defined in parsehelp.h
  Added here because they are in grammar.tab.h
*/

struct identlist;
class function;
class stack_machine;
#endif
