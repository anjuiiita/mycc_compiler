
#include "lexer.h"
#include "parsehelp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector>
#include <fstream>
#include <map>

extern int yylineno;
extern const char* filename;
extern const char* jvm_file;
extern FILE* jF;

/* ====================================================================== */

/*
  The function class can be hidden now
*/

class function {
    struct stmtnode {
      int lineno;
      typeinfo type;
      stmtnode* next;
    };

  private:
    typeinfo type;
    char* name;
    identlist* formals;
    identlist* locals;
    
    bool prototype_only;

    stmtnode* stmtList;
    stmtnode* stmtEnd;

    int lineno;

  public:
    function(typeinfo T, char* n, identlist* F);
    ~function();

    // Print an error message
    void redefinition() const;

    void set_proto(bool po);

    inline bool is_prototype() const { 
      return prototype_only;
    }

    inline bool name_matches(const char* findname) const {
      return (0==strcmp(name, findname));
    };

    bool params_match(const identlist* other) const;
    void replace_params(identlist* newformals);
    bool call_matches(const typelist* actual) const;

    // Return true on success, false on error because already defined
    bool addLocals(identlist* L);

    // Return true on success, false on error because already defined
    bool addStatement(int lineno, typeinfo type);

    /* Show information for modes 2 and 3 */
    void display(std::ostream& out, bool show_types) const;

    inline const identlist* find(const char* name) const
    {
      const identlist* v = locals ? locals->find(name) : 0;
      if (v) return v;
      return formals ? formals->find(name) : 0;
    }

    inline typeinfo getType() const { return type; }

    inline char* getName() const { return name; }

    inline identlist* getParams() const { return formals; }
    inline identlist* getLocals() const { return locals; }
};

/* ====================================================================== */

parse_data parse_data::THE_DATA;

void parse_data::Initialize(bool typecheck)
{
  THE_DATA.globals = 0;
  THE_DATA.functions = 0;
  THE_DATA.current_function = 0;
  THE_DATA.jvm = new stack_machine();
  THE_DATA.typechecking = typecheck;
}

void parse_data::Finalize()
{
  THE_DATA.functions = funclist::reverseList(THE_DATA.functions);
}

void parse_data::showGlobals(std::ostream &s)
{
  if ( (!TypecheckingOn()) && (0==THE_DATA.globals) ) return;
  s << "Global variables\n";
  THE_DATA.globals->display(s, TypecheckingOn());
  s << "\n";
}

void parse_data::showFunctions(std::ostream &s)
{
  for (funclist* curr = THE_DATA.functions; curr; curr=curr->next) {
    curr->F->display(s, TypecheckingOn());
  }
}

void parse_data::declareGlobals(identlist* L)
{
  THE_DATA.globals = identlist::Append(
    THE_DATA.globals,
    identlist::reverseList(L), 
    TypecheckingOn() ? "Global variable" : 0
  );
  fprintf(jF, "; Global vars\n");
  for (const identlist* curr = THE_DATA.globals; curr; curr=curr->next) {
        fprintf(jF, ".field public static %s %c\n", curr->name, curr->type.typecode);
  }
  fprintf(jF, "\n.method <init> : ()V\n");
  fprintf(jF, "\t.code stack 1 locals 1\n");
  fprintf(jF, "\t\taload_0\n");
  fprintf(jF, "\t\tinvokespecial Method java/lang/Object <init> ()V\n");
  fprintf(jF, "\t\treturn\n");
  fprintf(jF, "\t.end code\n");
  fprintf(jF, ".end method\n\n");
}

void parse_data::declareLocals(identlist* L)
{
  if (THE_DATA.current_function) {
    if (! THE_DATA.current_function->addLocals(L)) {
      THE_DATA.current_function = 0;
    } 
  } else {
    identlist::deleteList(L);
  }
} 

function* parse_data::startFunction(typeinfo T, char* n, identlist* P)
{
  THE_DATA.current_function = 0;

  P = identlist::reverseList(P);

  function* F = 0;

  if (TypecheckingOn()) {
      // Check for existing prototypes here

      F = THE_DATA.find(n);
      if (F) {
        if ( (F->getType() != T) || (! F->params_match(P)) ) {
          // parameters don't match.
          startError(yylineno);
          std::cerr << "Conflicting types for function " << n << "\n";

          free(n);
          identlist::deleteList(P);
          return 0;
        }

        // If the previous thing is only a prototype,
        // then replace the formal parameters.

        if (F->is_prototype()) {
          F->replace_params(P);
        }
        
        return (THE_DATA.current_function = F);
      }
  }

  F = new function(T, n, P);

  if (F) {
    funclist* L = new funclist(F);
    if (THE_DATA.functions) {
      THE_DATA.functions = THE_DATA.functions->Push(L);
    } else {
      THE_DATA.functions = L;
    }
  }

  return (THE_DATA.current_function = F);
}

void parse_data::startFunctionDef()
{
  if (THE_DATA.current_function) {
    if (THE_DATA.current_function->is_prototype()) {
      typeinfo t = THE_DATA.current_function->getType();
      identlist* i = THE_DATA.current_function->getParams();
      identlist* l = THE_DATA.current_function->getLocals();
      std::string params;
      while ( i != NULL) {
        if (i->is_array)
          params += "[";
        params += i->type.typecode;
        i = i->next;
      }
      fprintf(jF, ".method public static %s : (%s)%c\n", THE_DATA.current_function->getName(), params.c_str(), t.typecode);

      return;
    }
    THE_DATA.current_function->redefinition();
    THE_DATA.current_function = 0;
  } else {

  }
}

void parse_data::doneFunction(function* F, bool proto_only)
{
  if (F) {
    F->set_proto(proto_only);
  }
  if (!proto_only && F) {
    int count = 0;
    identlist* i = F->getParams();
    identlist* l = F->getLocals();
    while ( i != NULL) {
      count++;
      i = i->next;
    }
    int l_count = 0;
    while (l != NULL) {
      count++;
      l_count++;
      l = l->next;
    }
    fprintf(jF, "\t.code stack %d locals %d\n", THE_DATA.jvm->getStackDepth(), l_count);
    THE_DATA.jvm->show_stack();
  }

  THE_DATA.current_function = 0;
}

typeinfo parse_data::buildUnary(char op, typeinfo opnd)
{
  typeinfo answer;
  answer.set('E', 0);

  if (TypecheckingOn()) {

      switch (opnd.typecode) {
        case 'C' : 
        case 'I' :  answer = opnd;
                    break;

        case 'F' :  if ('~' != op) answer = opnd;
                    break;
      }
      if (opnd.is_array) answer.set('E', 0);

      if ('E' != answer.typecode) {
        if ('!' == op) answer.set('C', 0);
      }

      if ('E' == answer.typecode) {
        startError(yylineno);
        std::cerr << "Operation not supported: " << op << ' ' << opnd << "\n";
      }
  }

  return answer;
}

typeinfo parse_data::buildCast(typeinfo cast, typeinfo opnd)
{
  typeinfo answer;
  answer.set('E', 0);

  if (TypecheckingOn()) {
    
    if (opnd.is_number() && cast.is_number()) {
      answer = cast;
    } else {
      startError(yylineno);
      std::cerr << "Cannot cast from type " << opnd << " to type " << cast << "\n";
    }

  }

  return answer;
}

typeinfo parse_data::buildArith(typeinfo left, char op, typeinfo right)
{
    typeinfo answer;
    answer.set('E', 0);

    if (TypecheckingOn()) {

        if (left.typecode == right.typecode) {
            answer = left;
        }
        if (! answer.is_number()) {
            answer.set('E', 0);
        }

        if ('F' == answer.typecode) {

            if ( ('%' == op) || ('&' == op) || ('|' == op) ) {
            answer.set('E', 0);
            }
        }

        if ('E' == answer.typecode) {
            startError(yylineno);
            std::cerr << "Operation not supported: " << left << ' ' << op << ' ' << right << "\n";
        }
    }

    if (op == '+') {
        THE_DATA.jvm->machine_code.push_back("\t\tiadd\n");
    } 
    if (op == '-') {
        THE_DATA.jvm->machine_code.push_back("\t\tisub\n");
    } 
    if (op == '/') {
        THE_DATA.jvm->machine_code.push_back("\t\tidiv\n");
    } 
    if (op == '*') {
        THE_DATA.jvm->machine_code.push_back("\t\timul\n");
    } 
    if (op == '%') {
        THE_DATA.jvm->machine_code.push_back("\t\tirem\n");
    } 
    if (op == '|') {
        THE_DATA.jvm->machine_code.push_back("\t\tior\n");
    } 
    if (op == '&') {
        THE_DATA.jvm->machine_code.push_back("\t\tiand\n");
    }
    THE_DATA.jvm->pop_stack();
    THE_DATA.jvm->pop_stack();
    THE_DATA.jvm->decStackDepth();
    THE_DATA.jvm->decStackDepth();

  return answer;
}

typeinfo parse_data::buildLogic(typeinfo left, const char* op, typeinfo right)
{
  typeinfo answer;
  answer.set('E', 0);

  if (TypecheckingOn()) {

      if (left.typecode == right.typecode) {
        answer.set('C', 0);
      }
      if (! left.is_number() ) {
        answer.set('E', 0);
      }

      if ('E' == answer.typecode) {
        startError(yylineno);
        std::cerr << "Operation not supported: " << left << ' ' << op << ' ' << right << "\n";
      }

  }

  return answer;
}

typeinfo parse_data::buildIncDec(bool pre, char op, typeinfo opnd)
{
  typeinfo answer;
  answer.set('E', 0);
  
  if (TypecheckingOn()) {
      if (opnd.is_number()) {
        answer = opnd;
      }

      if ('E' == answer.typecode) {
        startError(yylineno);
        std::cerr << "Cannot ";
        if ('+' == op) std::cerr << "in"; else std::cerr << "de";
        std::cerr << "crement lvalue of type " << opnd << "\n";
      }
  }

  return answer;
}

typeinfo parse_data::buildUpdate(typeinfo lhs, const char* op, typeinfo rhs)
{
    typeinfo answer;
    answer.set('E', 0);

    if (TypecheckingOn()) {
        if (lhs.typecode == rhs.typecode) {
            answer = lhs;
        }
        if (! answer.is_number()) {
            answer.set('E', 0);
        }

        if ('E' == answer.typecode) {
            startError(yylineno);
            std::cerr << "Operation not supported: " << lhs << ' ' << op << ' ' << rhs << "\n";
        }
    }

    const identlist* var;
    std::string local_data = THE_DATA.jvm->peek_stack();
    if(THE_DATA.current_function)
        var = THE_DATA.current_function->find(local_data.c_str());

    if (!var && THE_DATA.globals) {
        var = THE_DATA.globals->find(local_data.c_str());
    }

    if (!var) {
        THE_DATA.jvm->pop_stack();
        THE_DATA.jvm->decStackDepth();
        local_data = THE_DATA.jvm->peek_stack();
        if(THE_DATA.current_function)
            var = THE_DATA.current_function->find(local_data.c_str());

        if (!var && THE_DATA.globals) {
            var = THE_DATA.globals->find(local_data.c_str());
        }
    }

    if (var) {
        THE_DATA.jvm->machine_code.push_back("\t\tdup\n");
        int dep = THE_DATA.jvm->getStackDepth();
        std::string cmd = "\t\t";

        
        if (var && lhs.typecode == 'I') {
            if (var->type.is_array) {
                cmd += "ia";
            } else {
                cmd += "i";
            }
            cmd += "store_" + std::to_string(dep) + " ; store to " + local_data + "\n";
            THE_DATA.jvm->local_pos[local_data] = dep;
        }
        if (var && lhs.typecode == 'C') {
            if (var && var->type.is_array) {
                cmd += "ca";
            } else {
                cmd += "c";
            }
            cmd += "store_" + std::to_string(dep) + " ; store to " + local_data + "\n";
            THE_DATA.jvm->local_pos[local_data] = dep;
        }
        if (var && lhs.typecode == 'F') {
            if (var && var->type.is_array) {
                cmd += "fa";
            } else {
                cmd += "f";
            }

            cmd += "store_" + std::to_string(dep) + " ; store to " + local_data + "\n";
            THE_DATA.jvm->local_pos[local_data] = dep;
        }
        THE_DATA.jvm->machine_code.push_back(cmd);
        THE_DATA.jvm->push_stack(THE_DATA.jvm->peek_stack());
        THE_DATA.jvm->push_stack(THE_DATA.jvm->peek_stack());
        THE_DATA.jvm->incStackDepth();
        THE_DATA.jvm->incStackDepth();

        if (strcmp(rhs.bytecode, (char*)"none") != 0) {
            THE_DATA.jvm->pop_stack();
            THE_DATA.jvm->decStackDepth();
        }
    }
    return answer;
}

typeinfo parse_data::buildTernary(typeinfo cond, typeinfo then, typeinfo els)
{
  typeinfo answer;
  answer.set('E', 0);

  if (TypecheckingOn()) {
      if (then.typecode == els.typecode) {
        answer = then;
      }
      if (! cond.is_number() ) {
        answer.set('E', 0);
      }

      if ('E' == answer.typecode) {
        startError(yylineno);
        std::cerr << "Operation not supported: " << cond;
        std::cerr << " ? " << then << " : " << els << "\n";
      }
  }

  return answer;
}

typeinfo parse_data::buildLval(char* ident, bool flag)
{
  typeinfo error;
  error.set('E', 0);

  if (TypecheckingOn()) {
    const identlist* var = 0;
    var = THE_DATA.current_function->find(ident);

    if (!var && THE_DATA.globals) {
      var = THE_DATA.globals->find(ident);
    }
    if (!var) {
      startError(yylineno);
      std::cerr << "Undeclared identifier: " << ident << "\n";
      free(ident);
      return error;
    }

    if (var && flag) {
        THE_DATA.jvm->push_stack(ident);
        THE_DATA.jvm->incStackDepth();
        int dep = THE_DATA.jvm->getStackDepth();
        int pos = THE_DATA.jvm->local_pos[std::string(ident)];
        if (var->type.typecode == 'I') {
          std::string cmd = "\t\t";
          if (var->type.is_array) {
            cmd += "ia";
          } else {
            cmd += "i";
          }
          cmd += "load_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
        if (var->type.typecode == 'C') {
          std::string cmd = "\t\t";
          if (var->type.is_array) {
            cmd += "ca";
          } else {
            cmd += "c";
          }
          cmd += "load_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
        if (var->type.typecode == 'F') {
          std::string cmd = "\t\t";
          if (var->type.is_array) {
            cmd += "fa";
          } else {
            cmd += "f";
          }
          cmd += "load_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
      }
      if (var && !flag) {
        THE_DATA.jvm->push_stack(ident);
      }

    free(ident);
    return var->type;
  }

  return error;
}

typeinfo parse_data::buildLvalBracket(char* ident, typeinfo index, bool flag)
{
  typeinfo error;
  error.set('E', 0);

  if (TypecheckingOn()) {
    const identlist* var = 0;
    if (!var && THE_DATA.globals) {
      var = THE_DATA.globals->find(ident);
    }
    if (!var) {
      startError(yylineno);
      std::cerr << "Undeclared identifier: " << ident << "\n";
      free(ident);
      return error;
    }

    if (!var->type.is_array) {
      startError(yylineno);
      std::cerr << "Identifier " << ident << " is not an array\n";
      free(ident);
      return error;
    }

    if ( (index.typecode != 'I') || (index.is_array) ) {
      startError(yylineno);
      std::cerr << "Array index should be an integer (was: " << index << ")\n";
      free(ident);
      return error;
    }
    
    if (var && flag) {
        THE_DATA.jvm->push_stack(ident);
        //THE_DATA.jvm->push_stack(index);
        //THE_DATA.jvm->incStackDepth();
        THE_DATA.jvm->incStackDepth();
        int dep = THE_DATA.jvm->getStackDepth();
        int pos = THE_DATA.jvm->local_pos[std::string(ident)];
        if (var->type.typecode == 'I') {
          std::string cmd = "\t\tiaload_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
        if (var->type.typecode == 'C') {
          std::string cmd = "\t\tload_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
        if (var->type.typecode == 'F') {
          std::string cmd = "\t\tfaload_" + std::to_string(pos) + " ; load from " + std::string(ident) + "\n";
          THE_DATA.jvm->machine_code.push_back(cmd);
        }
    }
    if (var && !flag) {
        THE_DATA.jvm->push_stack(ident);
    }
      
    free(ident);

    typeinfo answer = var->type;
    answer.is_array = false;
    return answer;
  }

  return error;
}

typeinfo parse_data::buildFcall(char* ident, typelist* params)
{
  typeinfo answer;
  answer.set('E', 0);

  if (TypecheckingOn()) {

    // Reverse params
    typelist* rev = 0;
    while (params) {
      typelist* next = params->next;
      params->next = rev;
      rev = params;
      params = next;
    }
    params = rev;

    // Now, make sure there's a function
    function* F = 0;
    F = THE_DATA.find(ident);

    if (F) {
      if (F->call_matches(params)) {
        // Good function call
        answer = F->getType();

        typelist* i = params;
        std::string params_type;
        while ( i != NULL) {
          if (i->type.is_array)
            params_type += "[";
          params_type += i->type.typecode;
          i = i->next;
        }
        std::string data = "\t\tinvokestatic Method libc " + std::string(ident) + " (" + params_type +")" + F->getType().typecode + "\n";
        THE_DATA.jvm->push_stack(data);
        THE_DATA.jvm->incStackDepth();
        THE_DATA.jvm->machine_code.push_back(data);

      } else {
        startError(yylineno);
        std::cerr << "Parameter mismatch in function call\n\t";
        std::cerr << ident << "(";
        for (const typelist* curr = params; curr; curr=curr->next) {
          if (curr != params) std::cerr << ", ";
          std::cerr << curr->type;
        }
        std::cerr << ")\n";
      }
    } else {
      startError(yylineno);
      std::cerr << "Function " << ident << " has not been declared.\n";
    }
  }

  free(ident);
  while (params) {
    typelist* next = params->next;
    delete params;
    params = next;
  }

  return answer; 
}

void parse_data::checkCondition(bool can_be_empty, const char* stmt, typeinfo cond, int lineno)
{
    THE_DATA.jvm->stack_mc.push_back("\t\t;; " + std::string(filename) + std::string(" ") + std::to_string(yylineno) + " expression\n");

    if (!TypecheckingOn()) return;
    if (cond.is_number()) return;
    if (can_be_empty) {
        if (' ' == cond.typecode) return;
    }

    startError(lineno);
    std::cerr << "Condition of " << stmt << " has invalid type: " << cond << "\n";
}

void parse_data::checkReturn(typeinfo type)
{

  if (!TypecheckingOn()) return;

  if (THE_DATA.current_function) {
    THE_DATA.jvm->stack_mc.push_back("\t\t;; " + std::string(filename) + std::string(" ") + std::to_string(yylineno) + " return\n");
    THE_DATA.jvm->stack_mc.insert(THE_DATA.jvm->stack_mc.end(), THE_DATA.jvm->machine_code.begin(), THE_DATA.jvm->machine_code.end());
    if (type.typecode == 'I') {
      THE_DATA.jvm->stack_mc.push_back("\t\tireturn\n");
    }
    if (type.typecode == 'F') {
      THE_DATA.jvm->stack_mc.push_back("\t\tfreturn\n");
    }
    if (type.typecode == 'C') {
      THE_DATA.jvm->stack_mc.push_back("\t\tcreturn\n");
    }
    
    typeinfo Ft = THE_DATA.current_function->getType();

    if (type != Ft) {
      startError(yylineno);
      std::cerr << "Returning " << type << " in a function of type " << Ft << "\n";
    }
  }
}

void parse_data::addExprStmt(typeinfo type)
{
    if (THE_DATA.current_function) {
        THE_DATA.current_function->addStatement(yylineno, type);
    }
    THE_DATA.jvm->stack_mc.push_back("\t\t;; " + std::string(filename) + std::string(" ") + std::to_string(yylineno) + " expression\n");
    THE_DATA.jvm->machine_code.push_back("\t\tpop\n");
    THE_DATA.jvm->stack_mc.insert(THE_DATA.jvm->stack_mc.end(), THE_DATA.jvm->machine_code.begin(), THE_DATA.jvm->machine_code.end());
    THE_DATA.jvm->machine_code.clear();
    THE_DATA.jvm->decStackDepth();
    THE_DATA.jvm->pop_stack();
}

function* parse_data::find(const char* name) const
{
  for (funclist* curr = functions; curr; curr = curr->next) {
    if (curr->F->name_matches(name)) return curr->F;
  }
  return 0;
}

void parse_data::show_machine_code() {
  FILE* file;
  std::string f = std::string(filename);
  const char * ft = f.replace(f.find(".c"), sizeof(".c") - 1, ".j").c_str();
  if ((file = fopen(ft, "r")) != NULL) {//checking whether the file is open
    char c;
    while ((c = getc(file)) != EOF) {
        printf("%c", c);
    }
    fclose(file);
  }
}

/* ====================================================================== */

parse_data::funclist::funclist(function *f)
{
  F = f;
  next = 0;
}

parse_data::funclist::~funclist()
{
  delete F;
}

parse_data::funclist* parse_data::funclist::Push(funclist* item)
{
  if (0==item) return this;

  funclist* tail = item;
  while (tail->next) tail=tail->next;
  tail->next = this;

  return item;
}

parse_data::funclist* parse_data::funclist::reverseList(funclist *L)
{
  funclist* newlist = 0;

  while (L) {
    funclist* curr = L;
    L = L->next;

    curr->next = newlist;
    newlist = curr;
  }

  return newlist;
}

void parse_data::load_stack(char* literal, bool flag) {
    if (!flag) {
        THE_DATA.jvm->push_stack(std::string(literal));
        THE_DATA.jvm->incStackDepth();
        THE_DATA.jvm->machine_code.push_back("\t\tbipush " + std::string(literal) + "\n");
    } else {
        //printf("%s\n", literal);
    }
}

typeinfo parse_data::buildLiteral(typeinfo val, bool flag) {
    std::cerr << val.bytecode << "\n";
    return val;
}

/* ====================================================================== */

std::ostream& operator<<(std::ostream& s, typeinfo T)
{
  switch (T.typecode) {
    case 'E':   return s << "error";
    case 'V':   return s << "void";   /* can't be array */
    case 'C':   s << "char";  break;
    case 'I':   s << "int";   break;
    case 'F':   s << "float"; break;
  }
  if (T.is_array) s << "[]";
  return s;
}

/* ====================================================================== */


identlist::identlist(typeinfo T, char* _name, bool array)
{
  type.set(T.typecode, array);
  name = _name;
  lineno = yylineno;
  is_array = array;
  next = 0;
}

identlist::identlist(char* _name, bool array)
{
  type.set('E');
  name = _name;
  lineno = yylineno;
  is_array = array;
  next = 0;
}

identlist::~identlist()
{
  free(name);
}

identlist* identlist::Push(identlist* item)
{
  if (0==item) return this;

  identlist* tail = item;
  while (tail->next) tail=tail->next;
  tail->next = this;

  return item;
}

void identlist::display(std::ostream& out, bool showtypes) const
{
  if (!showtypes) out << "    ";
  for (const identlist* curr = this; curr; curr=curr->next) {
    if (showtypes) {
      out << '\t' << curr->type << " ";
    }
    out << curr->name;
    if (showtypes) {
      out << "\n";
    } else {
      if (curr->is_array) out << "[]";
      if (curr->next) out << ", ";
    }
  }
  if (!showtypes) out << "\n";
}

const identlist* identlist::find(const char* name) const
{
  for (const identlist* curr = this; curr; curr = curr->next) {
    if (0==strcmp(name, curr->name)) return curr;
  }
  return 0;
}

identlist* identlist::reverseList(identlist* L)
{
  identlist* newlist = 0;

  while (L) {
    identlist* curr = L;
    L = L->next;

    curr->next = newlist;
    newlist = curr;
  }

  return newlist;
}

identlist* identlist::setTypes(typeinfo T, identlist* L)
{
  if (parse_data::TypecheckingOn() && 'V' == T.typecode) {
    L = reverseList(L);
    startError(yylineno);
    std::cerr << "Cannot declare void variable";
    if (L->next) std::cerr << "s";
    std::cerr << ": ";
    for (identlist* curr = L; curr; curr=curr->next) {
      if (curr != L) std::cerr << ", ";
      std::cerr << curr->name;
    }
    std::cerr << "\n";
    deleteList(L);
    return 0;
  }
  for (identlist* curr = L; curr; curr=curr->next) {
    curr->type.set(T.typecode, curr->is_array);
  }
  return L;
}

void identlist::deleteList(identlist* L)
{
  while (L) {
    identlist* next = L->next;
    delete L;
    L = next;
  }
}

identlist* identlist::Append(identlist* M, identlist* items, const char* wh)
{
  /*
    Again, not the fastest,
    but it gets the job done.
  */
  identlist* Mend = M;
  if (Mend) {
    while (Mend->next) Mend = Mend->next;
  }

  while (items) {
    identlist* nextitem = items->next;
    items->next = 0;
    bool duplicate = false;

    if (wh) {
      // Check M for duplicates
      for (identlist* curr = M; curr; curr = curr->next) {
        if (strcmp(curr->name, items->name)) continue;

        std::cerr << "Error near " << filename << " line " << items->lineno;
        std::cerr << ":\n\t" << wh << " " << items->name << " already declared.";
        std::cerr << "\n\t(Original is near " << filename << " line " << curr->lineno << ")\n";
        duplicate = true;
        break;
      }
    } 

    // Delete duplicate; otherwise add to end
    if (duplicate) {
      delete items;
    } else {

      if (Mend) Mend->next = items;
      else      M = items;
      Mend = items;
    }

    // Advance
    items = nextitem;
  }

  return M;
}

/* ====================================================================== */

function::function(typeinfo T, char* n, identlist* F)
{
  type = T;
  name = n;
  formals = identlist::Append(
    0, F, parse_data::TypecheckingOn() ? "Parameter" : 0
  );
  prototype_only = true;
  locals = 0;

  stmtList = 0;
  stmtEnd = 0;

  lineno = yylineno;
}

function::~function()
{
  free(name);
  while (formals) {
    identlist* N = formals->next;
    delete formals;
    formals = N;
  }

  while (stmtList) {
    stmtnode* next = stmtList->next;
    delete stmtList;
    stmtList = next;
  }
}

void function::redefinition() const
{
  startError(yylineno);
  std::cerr << "Function " << name << " redefined.\n\t(Original is near ";
  std::cerr << filename << " line " << lineno << ".)\n";
}

void function::set_proto(bool po)
{
  if (prototype_only) {
    prototype_only = po;
  }
}

bool function::params_match(const identlist* other) const
{
  const identlist* fcurr = formals;

  while (fcurr && other) {
    if (fcurr->type != other->type) {
      return false;
    }
    fcurr = fcurr->next;
    other = other->next;
  }

  if (fcurr) return false;
  if (other) return false;

  return true;
}

void function::replace_params(identlist* newformals)
{
  identlist::deleteList(formals);
  formals = newformals;
}

bool function::call_matches(const typelist* actual) const
{
  const identlist* fcurr = formals;

  while (fcurr && actual) {
    if (fcurr->type != actual->type) {
      return false;
    }
    fcurr = fcurr->next;
    actual = actual->next;
  }

  if (fcurr) return false;
  if (actual) return false;

  return true;
}

bool function::addLocals(identlist* L)
{
  assert(prototype_only);

  locals = identlist::Append(
    locals,
    identlist::reverseList(L), 
    parse_data::TypecheckingOn() ? "Local variable" : 0
  );

  return true;
}

bool function::addStatement(int lineno, typeinfo type)
{
  if (!prototype_only) {
    return false;
  }

  if ('E' == type.typecode) return true;
  if (' ' == type.typecode) return true;

  stmtnode* curr = new stmtnode;
  curr->lineno = lineno;
  curr->type = type;
  curr->next = 0;

  if (stmtList) {
    stmtEnd->next = curr;
  } else {
    stmtList = curr;
  }
  stmtEnd = curr;

  return true;
}

void function::display(std::ostream &out, bool show_types) const
{
  //out << name << " " << show_types << " " << prototype_only << " " << "\n";
  if (show_types && prototype_only) return;
  if (prototype_only) out << "Prototype ";
  else                out << "Function ";
  out << name;
  if (show_types) {
    out << ", returns " << type;
  }
  out << "\n";
  if (show_types || formals) {
    out << "    Parameters";
    if (show_types) out << "\n";
    else            out << ": ";
    formals->display(out, show_types);
    if (show_types) out << "\n";
  }
  if (show_types || locals) {
    out << "    Local variables";
    if (show_types) out << "\n";
    else            out << ": ";
    if (locals) locals->display(out, show_types);
    if (show_types) out << "\n";
  }
  if (show_types) {
    out << "    Statements\n";
    for (stmtnode* curr = stmtList; curr; curr = curr->next) {
      out << "\tExpression on line " << curr->lineno;
      out << " has type " << curr->type << "\n";
    }
  }
  out << "\n";
}

stack_machine::stack_machine() {
    stack_depth = 0;
    st = NULL;
}

stack_machine::~stack_machine() {
    while (st) {
        stack_code* next = st->next;
        delete st;
        st = next;
    }
}

void stack_machine::push_stack(std::string data)
{
    struct stack_code* temp;
    temp = (struct stack_code*)malloc(sizeof(struct stack_code));

    if (!temp)
    {
        exit(1);
    }

    temp->data = data;
    temp->next = st;
    st = temp;
}
 
void stack_machine::pop_stack()
{
    struct stack_code* temp;
 
    if (st == NULL)
    {
        printf("stack is empty\n");
        exit(1);
    }
    else
    {
        temp = st;
        st = st->next;
        temp->next = NULL;
        free(temp);
    }
}

bool stack_machine::isStackEmpty()
{
    return st == NULL;
}

std::string stack_machine::peek_stack()
{
    if (!isStackEmpty())
        return st->data;
    else {
        printf("stack is empty\n");
        exit(1);
    }
}

void stack_machine::show_stack()
{
  std::vector<std::string>::iterator it;
  for (it = stack_mc.begin(); it != stack_mc.end(); ++it) {
    fprintf(jF, "%s", (*it).c_str());
  }
}
