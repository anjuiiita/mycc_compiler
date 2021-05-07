
/*
  
  Lex / Flex file for generating tokens

*/

%{

#include "lexer.h"
#include "parsehelp.h"
#include "grammar.tab.h"  /* tokens defined here */

/*
  Just get tokens (mode 1)?
*/
extern char tokens_only;
extern int loop;
extern int if_flag;

/*
  Keep track of when a comment starts
  so we can give an error on unclosed comments
*/
int start_comment;

extern const char* filename;
extern const char* jvm_file;
%}

%option yylineno

%x COMMENT

ws                [\r\t ]
white             ({ws}*)
comstart      "/*"
comclose      "*"+"/"
notspecial    ([^*/])
notendl       [^\n]

letter            [A-Za-z_]
digit             [0-9]
alphanum          [A-Za-z_0-9]

dec           ("."{digit}+)
exp           ([eE][-+]?{digit}+)
qstring       (\"([^\"\n]|"\\\"")*\")
qchar         (\'[^\'\n]\')
qspecial      (\'\\.\')

ident         {letter}{alphanum}*

%%

"/*"                  { start_comment = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"         { BEGIN(INITIAL); }
<COMMENT>[^*\n]*      { /* Inside C comment */ }
<COMMENT>.            { /* Inside C comment */ }
<COMMENT>\n           { /* Inside C comment */ }
<COMMENT><<EOF>>      { return unclosedComment(start_comment); }
"//"{notendl}*        { /* C++ comment, ignored */ }
\n                    { /* Ignored */ }
{white}               { /* Ignored */ }

"#include"{white}{qstring}    { ignoringDirective("#include"); }
"#define"{notendl}*           { ignoringDirective("#define"); }
"#undef"{white}{ident}        { ignoringDirective("#undef"); }
"#ifdef"{white}{ident}        { ignoringDirective("#ifdef"); }
"#ifndef"{white}{ident}       { ignoringDirective("#ifndef"); }
"#else"                       { ignoringDirective("#else"); }
"#endif"                      { ignoringDirective("#endif"); }

"void"                { yylval.type.set('V'); yylval.type.setBytecode(strdup(yytext));  return TYPE; }
"int"                 { yylval.type.set('I'); yylval.type.setBytecode(strdup(yytext));  return TYPE; }
"char"                { yylval.type.set('C'); yylval.type.setBytecode(strdup(yytext));  return TYPE; }
"float"               { yylval.type.set('F'); yylval.type.setBytecode(strdup(yytext));  return TYPE; }

"const"               { return CONST; }
"struct"              { return STRUCT; }

"for"                 { return FOR; }
"while"               { loop = 1; return WHILE; }
"do"                  { return DO; }
"if"                  { if_flag = 1; return IF; }
"else"                { return ELSE; }
"break"               { return BREAK; }
"continue"            { return CONTINUE; }
"return"              { return RETURN; }

{digit}+              { yylval.type.set('I', false); yylval.type.setBytecode(strdup(yytext)); return INTCONST; }
{digit}+{dec}?{exp}?  { yylval.type.set('F', false); yylval.type.setBytecode(strdup(yytext)); return REALCONST; }
{dec}{exp}?           { yylval.type.set('F', false); yylval.type.setBytecode(strdup(yytext)); return REALCONST; }
{ident}               { if (!tokens_only) { yylval.name = strdup(yytext); }
                        return IDENT; 
                      }
{qstring}             { yylval.type.set('C', true); yylval.type.setBytecode(strdup(yytext)); return STRCONST; }
{qchar}               { yylval.type.set('C', false); yylval.type.setBytecode(strdup(yytext)); return CHARCONST; }
{qspecial}            { yylval.type.set('C', false); yylval.type.setBytecode(strdup(yytext)); return CHARCONST; }

"("                   { return LPAR; }
")"                   { return RPAR; }
"["                   { return LBRACKET; }
"]"                   { return RBRACKET; }
"{"                   { return LBRACE; }
"}"                   { return RBRACE; }

"."                   { return DOT; }
","                   { return COMMA; }
";"                   { return SEMI; }
"?"                   { return QUEST; }
":"                   { return COLON; }

"+"                   { yylval.type.setBytecode("plus"); return PLUS; }
"-"                   { yylval.type.setBytecode("sub"); return MINUS; }
"*"                   { yylval.type.setBytecode("mul"); return STAR; }
"/"                   { yylval.type.setBytecode("div"); return SLASH; }
"%"                   { yylval.type.setBytecode("rem"); return MOD; }
"~"                   { return TILDE; }

"|"                   { yylval.type.setBytecode("or"); return PIPE; }
"&"                   { yylval.type.setBytecode("and"); return AMP; }
"!"                   { return BANG; }
"||"                  { return DPIPE; }
"&&"                  { return DAMP; }

"="                   { return ASSIGN; }
"+="                  { return PLUSASSIGN; }
"-="                  { return MINUSASSIGN; }
"*="                  { return STARASSIGN; }
"/="                  { return SLASHASSIGN; }
"++"                  { return INCR; }
"--"                  { return DECR; }

"=="                  { yylval.type.setBytecode("if_icmpeq"); return EQUALS; }
"!="                  { yylval.type.setBytecode("if_icmpne"); return NEQUAL; }
">"                   { yylval.type.setBytecode("if_icmpge"); return GT; }
">="                  { yylval.type.setBytecode("if_icmpge"); return GE; }
"<"                   { yylval.type.setBytecode("if_icmplt"); return LT; }
"<="                  { yylval.type.setBytecode("if_icmple"); return LE; }

.                     { badToken(yytext); }

