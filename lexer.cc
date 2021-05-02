
#include "lexer.h"
#include "parsehelp.h"
#include "grammar.tab.h"    /* Tokens defined here */

/* 
  Global variables, sorry
*/

const char* filename;
const char* jvm_file;
std::string classname;
FILE * jF;
char tokens_only;
extern int yylineno;          /* flex manages this */
extern const char* yytext;    /* flex also manages this */
char last_mode;
int loop;

void yyset_in(FILE*);         /* flex gives this */

// #define STOP_ERRORS 25

#ifdef STOP_ERRORS
unsigned total_errors;
#endif


int  initLexer(const char* infile, char _tok_only, char _last_mode)
{
#ifdef STOP_ERRORS
  total_errors = 0;
#endif
  last_mode = _last_mode;
  loop = 0;
  filename = infile;
  std::string jvm_code(infile);
  classname = jvm_code.replace(jvm_code.find(".c"), sizeof(".c") - 1, "");
  jvm_file = jvm_code.append(".j").c_str();
  jF = fopen(jvm_file, "w" );
  fprintf(jF, "\n; Java assembly code\n\n");
  fprintf(jF, ".class public %s\n", classname.c_str());
  fprintf(jF, ".super java/lang/Object\n\n");
  fprintf(jF, "; Global vars\n");
  fprintf(jF, "\n.method <init> : ()V\n");
  fprintf(jF, "\t.code stack 1 locals 1\n");
  fprintf(jF, "\t\taload_0\n");
  fprintf(jF, "\t\tinvokespecial Method java/lang/Object <init> ()V\n");
  fprintf(jF, "\t\treturn\n");
  fprintf(jF, "\t.end code\n");
  fprintf(jF, ".end method\n\n");
  tokens_only = _tok_only;
  yylineno = 1;
  FILE* fin = fopen(infile, "r");
  if (0==fin) {
    std::cerr << "Error, couldn't open input file " << infile << "\n";
    return 0;
  }
  yyset_in(fin);
  return 1;
}

void printLocation(std::ostream &out)
{
  out << filename << " line " << yylineno << " text '" << yytext << "'";
}

void startError()
{
#ifdef STOP_ERRORS
  if (++total_errors > STOP_ERRORS) {
    std::cerr << "Too many errors; exiting.\n";
    exit(1);
  }
#endif
  std::cerr << "Error near ";
  printLocation(std::cerr);
  std::cerr << "\n\t";
}

void startError(int lineno)
{
#ifdef STOP_ERRORS
  if (++total_errors > STOP_ERRORS) {
    std::cerr << "Too many errors; exiting.\n";
    exit(1);
  }
#endif
  std::cerr << "Error near " << filename << " line " << lineno << "\n\t";
}

int unclosedComment(int start)
{
  startError(start);
  std::cerr << "Unclosed comment\n";
  return 0;
}

void ignoringDirective(const char* dir)
{
  std::cerr << "Warning: ignoring " << dir << " directive in ";
  std::cerr << filename << " line " << yylineno << "\n";
}

void badToken(const char* x)
{
  startError();
  std::cerr << "unexpected characters; ignoring.\n";
}

int yywrap()
{
  /* Switch to next file.  Return 0 if more files, 1 if not. */
  return 1;
}

/*
  Epic Hack right here.
*/
#define CASE_RETURN(X)    case X : return #X

const char* getTokenName(int tok)
{
  switch (tok) {
    CASE_RETURN(TYPE);
    CASE_RETURN(CONST);
    CASE_RETURN(STRUCT);
    CASE_RETURN(FOR);
    CASE_RETURN(WHILE);
    CASE_RETURN(DO);
    CASE_RETURN(IF);
    CASE_RETURN(ELSE);
    CASE_RETURN(BREAK);
    CASE_RETURN(CONTINUE);
    CASE_RETURN(RETURN);

    CASE_RETURN(IDENT);
    CASE_RETURN(INTCONST);
    CASE_RETURN(REALCONST);
    CASE_RETURN(STRCONST);
    CASE_RETURN(CHARCONST);

    CASE_RETURN(LPAR);
    CASE_RETURN(RPAR);
    CASE_RETURN(LBRACKET);
    CASE_RETURN(RBRACKET);
    CASE_RETURN(LBRACE);
    CASE_RETURN(RBRACE);

    CASE_RETURN(DOT);
    CASE_RETURN(COMMA);
    CASE_RETURN(SEMI);
    CASE_RETURN(QUEST);
    CASE_RETURN(COLON);

    CASE_RETURN(PLUS);
    CASE_RETURN(MINUS);
    CASE_RETURN(STAR);
    CASE_RETURN(SLASH);
    CASE_RETURN(MOD);
    CASE_RETURN(TILDE);

    CASE_RETURN(PIPE);
    CASE_RETURN(AMP);
    CASE_RETURN(BANG);
    CASE_RETURN(DPIPE);
    CASE_RETURN(DAMP);

    CASE_RETURN(ASSIGN);
    CASE_RETURN(PLUSASSIGN);
    CASE_RETURN(MINUSASSIGN);
    CASE_RETURN(STARASSIGN);
    CASE_RETURN(SLASHASSIGN);
    CASE_RETURN(INCR);
    CASE_RETURN(DECR);

    CASE_RETURN(EQUALS);
    CASE_RETURN(NEQUAL);
    CASE_RETURN(GT);
    CASE_RETURN(GE);
    CASE_RETURN(LT);
    CASE_RETURN(LE);

    default:  return "(unknown token)";
  };
}

