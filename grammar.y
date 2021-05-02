
%{

#include "lexer.h"
#include "parsehelp.h"

void yyerror(const char* s)
{
  startError();
  fputs(s, stderr);
  fputc('\n', stderr);
}

extern int yylineno;
extern const char* filename;
extern const char* jvm_file;
extern std::string classname;
extern FILE* jF;
extern char last_mode;
extern int loop;

%}

%union {
  typeinfo type;
  char* name;
  identlist* idlist;
  function* func;
  typelist* plist;
  int lineno;
}



%token <name> IDENT;
%token <type> TYPE;

%token CHARCONST INTCONST REALCONST STRCONST 
CONST STRUCT FOR WHILE DO IF ELSE BREAK CONTINUE RETURN
SWITCH CASE DEFAULT
LPAR RPAR LBRACKET RBRACKET LBRACE RBRACE COMMA DOT SEMI
EQUALS NEQUAL GT GE LT LE
ASSIGN PLUSASSIGN MINUSASSIGN STARASSIGN SLASHASSIGN INCR DECR
PLUS MINUS STAR SLASH MOD COLON QUEST TILDE PIPE AMP BANG DPIPE DAMP

%type <idlist> ideclist idec vardecl formal fplist
%type <func> funcdecl
%type <type> literal expression exprorempty lvalue EXPWHILE
%type <plist> paramlist
%type <lineno> getlineno marker ifmarker ifnomarker turnloopOff getlinenoloop

%nonassoc WITHOUT_ELSE
%nonassoc ELSE

%left COMMA
%right ASSIGN PLUSASSIGN MINUSASSIGN STARASSIGN SLASHASSIGN
%right QUEST COLON
%left DPIPE
%left DAMP
%left PIPE
%left AMP
%left EQUALS NEQUAL
%left GT GE LT LE
%left PLUS MINUS
%left STAR SLASH MOD
%right BANG TILDE UMINUS INCR DECR
%left LPAR RPAR LBRACKET RBRACKET

/* %define parse.error verbose */

/*-----------------------------------------------------------------*/

%%

prog
    : program
      {
        if (last_mode == '5') {
          fprintf(jF, ".method public static main : ([Ljava/lang/String;)V\n");
          fprintf(jF, "\t.code stack 1 locals 1\n");
          fprintf(jF, "\t\tinvokestatic Method %s main ()I\n", classname.c_str());
          fprintf(jF, "\t\tpop\n");
          fprintf(jF, "\t\treturn\n");
          fprintf(jF, "\t.end code\n");
          fprintf(jF, ".end method\n");
        } else {
          fprintf(jF, ".method public static main : ([Ljava/lang/String;)V\n");
          fprintf(jF, "\t.code stack 2 locals 2\n");
          fprintf(jF, "\t\tinvokestatic Method %s main ()I\n", classname.c_str());
          fprintf(jF, "\t\tistore_1\n");
          fprintf(jF, "\t\tgetstatic Field java/lang/System out Ljava/io/PrintStream;\n");
          fprintf(jF, "\t\tldc 'Return code: '\n");
          fprintf(jF, "\t\tinvokevirtual Method java/io/PrintStream print (Ljava/lang/String;)V\n");
          fprintf(jF, "\t\tgetstatic Field java/lang/System out Ljava/io/PrintStream;\n");
          fprintf(jF, "\t\tiload_1\n");
          fprintf(jF, "\t\tinvokevirtual Method java/io/PrintStream println (I)V\n");
          fprintf(jF, "\t\treturn\n");
          fprintf(jF, "\t.end code\n");
          fprintf(jF, ".end method\n");
        }
        
        fclose(jF);
      }
    ;

program
    : /* empty */
    | program progitem      
    ;

progitem
    : vardecl
      {
        parse_data::declareGlobals($1);
        parse_data::initGlobal();
      }
    | prototype
    | funcdef
    ;

vardecl
    : TYPE ideclist SEMI
      {
        $$ = identlist::setTypes($1, $2);
      }
    ;

ideclist
    : ideclist COMMA idec
      {
        $$ = $1->Push($3); 
      }
    | idec
      {
        $$ = $1;
      }
    ;

idec
    : IDENT
      {
        $$ = new identlist($1, 0);
      }
    | IDENT LBRACKET literal RBRACKET
      {
        $$ = new identlist($1, 1);
      }
    ;

funcdecl
    : TYPE IDENT LPAR RPAR
      {
        $$ = parse_data::startFunction($1, $2, 0);
      }
    | TYPE IDENT LPAR fplist RPAR
      {
        $$ = parse_data::startFunction($1, $2, $4);
      }
    ;

fplist
    : formal
      {
        $$ = $1;
      }
    | fplist COMMA formal
      {
        $$ = $1->Push($3);
      }
    ;

formal
    : TYPE IDENT
      {
        $$ = new identlist($1, $2, false);
      }
    | TYPE IDENT LBRACKET RBRACKET
      {
        $$ = new identlist($1, $2, true);
      }
    ;

prototype
    : funcdecl SEMI
      {
        parse_data::doneFunction($1, true);
      }
    ;

funcdef
    : funcdecl startfuncdef LBRACE vardeclist statements RBRACE
      {  
        parse_data::doneFunction($1, false);
        fprintf(jF, "\t.end code\n");
        fprintf(jF, ".end method\n\n");
      }
    ;

startfuncdef
    : /* empty */
      {
        parse_data::startFunctionDef();
      }
    ;

vardeclist
    : /* empty */
    | vardeclist vardecl
      {
        parse_data::declareLocals($2);
      }
    ;

statements
    : /* empty */
    | statements statement
    ;

stmtblock
    : LBRACE statements RBRACE
    ;

stmtorblock
    : statement 
    | stmtblock  { loop = 0;}
    ;

exprorempty
    : /* empty */
      {
        $$.set(' ', false);
      }
    | expression
      {
        $$ = $1;
      }
    ;

statement
    : exprorempty SEMI
      {
        parse_data::addExprStmt($1);
      }
    | BREAK SEMI
    | CONTINUE SEMI
    | RETURN SEMI
      {
        parse_data::checkEmptyReturn();
      }
    | RETURN expression SEMI
      {
        parse_data::checkReturn($2);
      }
    | IF LPAR expression getlineno RPAR stmtorblock ifnomarker %prec WITHOUT_ELSE
      {
        parse_data::checkCondition(false, "if statement", $3, $4, "if");
      }
    | IF LPAR expression getlineno RPAR stmtorblock ifmarker ELSE stmtorblock ifnomarker
      {
        parse_data::checkCondition(false, "if statement", $3, $4, "ifelse");
      }
    | FOR LPAR exprorempty SEMI exprorempty getlineno SEMI exprorempty RPAR stmtorblock
      {
        parse_data::checkCondition(true, "for loop", $5, $6, "for");
      }
    | WHILE LPAR getlinenoloop expression turnloopOff RPAR stmtorblock marker
      {
        parse_data::checkCondition(false, "while loop", $4, $3, "while");
      }
    | WHILE LPAR getlinenoloop EXPWHILE turnloopOff RPAR stmtorblock marker
      {
        parse_data::checkCondition1(false, "while loop", $4, $3, "while");
      }
    | DO stmtorblock WHILE LPAR expression getlineno RPAR
      {
        parse_data::checkCondition(false, "do while loop", $5, $6, "dowhile");
      }
    ;

getlineno
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
      }
    ;

getlinenoloop
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
        parse_data::push_label();
      }
    ;

turnloopOff
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
        loop = 0;
      }
    ;

marker
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
        parse_data::loop_end_label();
      }
    ;
ifmarker
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
        parse_data::ifmarker();
      }
    ;
ifnomarker
    : /* empty but allows us to grab the line number at a specific point */
      {
        $$ = yylineno;
        parse_data::ifnomarker();
      }
    ;


EXPWHILE
    : IDENT 
      { 
        parse_data::loop_exp_marker();
      }

expression
    : literal
    | IDENT 
      { 
        $$ = parse_data::buildLval($1, true);
      }
    | IDENT LBRACKET expression RBRACKET
      { 
        $$ = parse_data::buildLvalBracket($1, $3, true);
      }
    | IDENT LPAR RPAR
      { 
        $$ = parse_data::buildFcall($1, 0);
      }
    | IDENT LPAR paramlist RPAR
      { 
        $$ = parse_data::buildFcall($1, $3);
      }
    | lvalue ASSIGN expression
      {
        $$ = parse_data::buildUpdate($1, "=", $3);
      }
    | lvalue PLUSASSIGN expression
      {
        $$ = parse_data::buildUpdate($1, "+=", $3);
      }
    | lvalue MINUSASSIGN expression
      {
        $$ = parse_data::buildUpdate($1, "-=", $3);
      }
    | lvalue STARASSIGN expression
      {
        $$ = parse_data::buildUpdate($1, "*=", $3);
      }
    | lvalue SLASHASSIGN expression
      {
        $$ = parse_data::buildUpdate($1, "/=", $3);
      }
    | lvalue INCR
      {
        $$ = parse_data::buildIncDec(0, '+', $1);
      }
    | lvalue DECR
      {
        $$ = parse_data::buildIncDec(0, '-', $1);
      }
    | INCR lvalue
      {
        $$ = parse_data::buildIncDec(1, '+', $2);
      }
    | DECR lvalue
      {
        $$ = parse_data::buildIncDec(1, '-', $2);
      }
    | MINUS expression %prec UMINUS
      {
        $$ = parse_data::buildUnary('-', $2);
      }
    | BANG expression
      {
        $$ = parse_data::buildUnary('!', $2);
      }
    | TILDE expression
      {
        $$ = parse_data::buildUnary('~', $2);
      }
    | expression EQUALS expression
      {
        $$ = parse_data::buildLogic($1, "==", $3);
      }
    | expression NEQUAL expression
      {
        $$ = parse_data::buildLogic($1, "!=", $3);
      }
    | expression GT expression
      {
        $$ = parse_data::buildLogic($1, ">", $3);
      }
    | expression GE expression
      {
        $$ = parse_data::buildLogic($1, ">=", $3);
      }
    | expression LT expression
      {
        $$ = parse_data::buildLogic($1, "<", $3);
      }
    | expression LE expression
      {
        $$ = parse_data::buildLogic($1, "<=", $3);
      }
    | expression PLUS expression
      {
        $$ = parse_data::buildArith($1, '+', $3);
      }
    | expression MINUS expression
      {
        $$ = parse_data::buildArith($1, '-', $3);
      }
    | expression STAR expression
      {
        $$ = parse_data::buildArith($1, '*', $3);
      }
    | expression SLASH expression
      {
        $$ = parse_data::buildArith($1, '/', $3);
      }
    | expression MOD expression
      {
        $$ = parse_data::buildArith($1, '%', $3);
      }
    | expression PIPE expression
      {
        $$ = parse_data::buildArith($1, '|', $3);
      }
    | expression AMP expression
      {
        $$ = parse_data::buildArith($1, '&', $3);
      }
    | expression DPIPE expression
      {
        $$ = parse_data::buildLogic($1, "||", $3);
      }
    | expression DAMP expression
      {
        $$ = parse_data::buildLogic($1, "&&", $3);
      }
    | expression QUEST expression COLON expression
      {
        $$ = parse_data::buildTernary($1, $3, $5);
      }
    | LPAR TYPE RPAR expression
      {
        $$ = parse_data::buildCast($2, $4);
      }
    | LPAR expression RPAR
      {
        $$ = $2;
      }
    ;

paramlist
    : expression
      {
        $$ = new typelist($1, 0);
      }
    | paramlist COMMA expression
      {
        $$ = new typelist($3, $1);  /* Reverse order */
      }
    ;

literal
    : INTCONST    
      {
        //$$.set('I', false);
        parse_data::load_stack($$.bytecode, false);
      }
    | REALCONST   
      {
        //$$.set('F', false);
        parse_data::load_stack($$.bytecode, false);
      }
    | STRCONST    
      {
        //$$.set('C', true);
        parse_data::load_stack($$.bytecode, true);
      }
    | CHARCONST   
      {
        //$$.set('C', false);
        parse_data::load_stack($$.bytecode, false);
      }
    ;

lvalue
    : IDENT
      { 
        $$ = parse_data::buildLval($1, false);
      }
    | IDENT LBRACKET expression RBRACKET
      { 
        $$ = parse_data::buildLvalBracket($1, $3, false);
      }
    ;

