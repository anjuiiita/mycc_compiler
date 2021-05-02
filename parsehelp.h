
#ifndef PARSEHELP_H
#define PARSEHELP_H

#include <iostream>
#include <string.h>
#include <vector>
#include <map>

#define MAX 1000
#define YYDEBUG 1
/* ======================================================================
  
  Classes.

====================================================================== */

struct identlist;
class function;
class stack_machine;

struct typeinfo {
    /*  
        'E': error
        ' ': missing b/c there's no expression
        'V': void
        'C': char
        'I': int
        'F': float
    */
    char typecode;
    char* bytecode;
    /*
        Is this an array of the given type?
    */
    bool is_array;
  public:
    inline void set(char tc, bool a=false) {
      typecode = tc; 
      is_array = a;
      bytecode = (char*)"none";
    }

    inline void setBytecode(std::string bc) {
      bytecode = new char[bc.length() + 1];
      strcpy(bytecode, bc.c_str());
    }

    inline bool operator==(const typeinfo T) const {
      return ( (typecode == T.typecode) && (is_array == T.is_array) );
    }

    inline bool operator!=(const typeinfo T) const {
      return ( (typecode != T.typecode) || (is_array != T.is_array) );
    }

    inline bool is_number() const {
      if (is_array) return false;
      return ('C' == typecode) || ('I' == typecode) || ('F' == typecode);
    }

    inline bool non_empty() const {
      return (' ' != typecode);
    }
};

std::ostream& operator<<(std::ostream&, typeinfo);


struct typelist {
    typeinfo type;
    typelist* next;
  public:
    typelist(typeinfo T, typelist* N) {
      type = T;
      next = N;
    }
};

struct identlist {
    typeinfo type;
    char* name;
    bool is_array;
    int lineno;
    identlist* next;
    bool is_global;

  public:
    identlist(typeinfo T, char* _name, bool array);
    identlist(char* _name, bool array);
    ~identlist(); /* does not delete next pointer */

    /* Put all of L in front of us */
    identlist* Push(identlist* L);

    void display(std::ostream& out, bool showtypes) const;

    /*
      Slow but gets the job done
    */
    const identlist* find(const char* name) const;

  public: 
    /*
      Helper methods
    */
    static identlist* reverseList(identlist* L);
    static identlist* setTypes(typeinfo T, identlist* L);
    static void deleteList(identlist* L);

    /*
      Append list "items" to the end of list "main".
      If dup_error is non-null, then we check for duplicates
      and give an error of 'dup_type' item already declared.
    */
    static identlist* Append(identlist* main, identlist* items, const char* dup_type);
};

class stack_machine {
    struct stack_code {
        std::string data;
        int depth;
        stack_code *next;
    };

    private:
        stack_code *st;
        int stack_depth;

    public:
        stack_machine();
        ~stack_machine();

        std::vector<std::string> machine_code;
        std::vector<std::string> stack_mc;
        std::map<std::string, int> local_pos;
        void push_stack(std::string data);
        void pop_stack();
        std::string peek_stack();
        bool isStackEmpty();
        void show_stack();
        inline int getStackDepth() {
            return stack_depth;
        }
        inline void incStackDepth() {
            stack_depth++;
        }
        inline void decStackDepth() {
            stack_depth--;
        }
};


class parse_data {
    static parse_data THE_DATA;
  public:
    /*
      These methods are called in mycc.cc
    */

    /*
      Call this before calling yyparse()
    */
    static void Initialize(bool typecheck);

    /*
      Call this after calling yyparse()
    */
    static void Finalize();

    /*
      Display global variables.
      Used in modes 2 and 3.
    */
    static void showGlobals(std::ostream &s);

    /*
      Display function information.
      Used in modes 2 and 3.
    */
    static void showFunctions(std::ostream &s);

  public:
    /*
      Methods below here are called in grammar.y
    */

    static void declareGlobals(identlist* L);
    static void declareLocals(identlist* L);

    static function* startFunction(typeinfo T, char* n, identlist* F);
    static void startFunctionDef();
    static void doneFunction(function* F, bool proto_only);


    static typeinfo buildUnary(char op, typeinfo opnd);
    static typeinfo buildCast(typeinfo cast, typeinfo opnd);
    static typeinfo buildArith(typeinfo left, char op, typeinfo right);
    static typeinfo buildLogic(typeinfo left, const char* op, typeinfo right);
    static typeinfo buildIncDec(bool pre, char op, typeinfo opnd);
    static typeinfo buildUpdate(typeinfo lhs, const char* op, typeinfo rhs);
    static typeinfo buildTernary(typeinfo cond, typeinfo then, typeinfo els);

    static typeinfo buildLiteral(typeinfo val, bool flag);
    static typeinfo buildLval(char* ident, bool flag);
    static typeinfo buildLvalBracket(char* ident, typeinfo index, bool flag);
    static typeinfo buildFcall(char* ident, typelist* params);
    static identlist* getNextIdentVar(std::string local_data);
    static void initGlobal();

    static void checkCondition(bool can_be_empty, const char* stmt, typeinfo cond, int lineno, const char* flag);
    static typeinfo checkCondition1(bool can_be_empty, const char* stmt, typeinfo cond, int lineno, const char* flag);
    static void checkReturn(typeinfo type);
    static void push_label();
    static void loop_end_label();
    static void ifmarker();
    static void ifnomarker();
    static void checkEmptyReturn();
    static typeinfo loop_exp_marker(char* ident);
    /*{
      typeinfo T;
      T.set('V', 0);
      checkReturn(T);
      THE_DATA.jvm->stack_mc.push_back("\t\treturn\n");
      THE_DATA.current_function->return_flag = 1;
    }*/

    static void addExprStmt(typeinfo type);
    static void load_stack(char* literal, bool flag);
    static void show_machine_code();
  
  private:
    struct funclist {
        function* F;
        funclist* next;
      public:
        funclist(function *f);
        ~funclist();  /* does not delete next pointer */

        funclist* Push(funclist* L);

      public:
        static funclist* reverseList(funclist*);
    };

  private:
    bool typechecking;
    identlist* globals;
    funclist* functions;
    function* current_function;
    stack_machine* jvm;

  private:
    function* find(const char* name) const;

  public:
    inline static bool TypecheckingOn() { return THE_DATA.typechecking; }
};

/*
  Generated by bison.
*/

int yyparse();





#endif

