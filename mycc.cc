
#include <iostream>
#include <fstream>

#include "lexer.h"
#include "parsehelp.h"

using namespace std;

int usage(const char* arg)
{
  if (arg) {
    cerr << "Unknown switch: " << arg << "\n\n";
  }
  cerr << "Usage:\n";
  cerr << "\tmycc -mode [options] infile\n";
  cerr << "\n";
  cerr << "Valid modes:\n";
  cerr << "\t -0: Version information\n";
  for (char mode='1'; mode <= '5'; mode++) {
    cerr << "\t -" << mode << ": Part " << mode << " (not implemented yet)\n";
  }
  cerr << "\n";
  cerr << "Valid options:\n";
  cerr << "\t -o outfile: write to outfile instead of standard output\n";
  cerr << "\n";
  return arg ? 1 : 0;
}

int version(ostream &s)
{
  s << "My bare-bones C compiler (for COM 440/540)\n";
  s << "\tWritten by Anju Kumari (ank@iastate.edu)\n";
  s << "\tVersion 0.1\n";
  s << "\t5 January, 2021\n";
  return 0;
}

int dump_tokens(ostream &s)
{
  for (int tok=yylex(); tok; tok=yylex()) {
    printLocation(s);
    s << " token " << getTokenName(tok) << "\n";
    s.flush();
    // for (int  i=1; i<300000000; i++);    // Add delay for testing script :)
  }
  return 0;
}

int compile(char mode, const char* infile, ostream &fout)
{
  if (' ' == mode) {
    cerr << "No mode specified; run without arguments for usage.\n";
    return 6;
  }

  if ('0'==mode) {
    if (infile) {
      cerr << "Warning: ignoring input file\n";
    }
    return version(fout);
  }

  if (0==infile) {
    cerr << "Error, no input file(s) specified.  Nothing to do.\n";
    return 7;
  }


  initLexer(infile, '1' == mode, '5' == mode, '4' == mode);
  

  if ('1'==mode) {
    return dump_tokens(fout);
  }

  parse_data::Initialize(mode > '2');
  yyparse();
  // We could catch the return of yyparse() to know
  // if a syntax error occurred or not.
  parse_data::Finalize();

  if ( ('2' == mode) || ('3' == mode) ) {
    parse_data::showGlobals(fout);
    parse_data::showFunctions(fout);
    return 0;
  }
  
   
   
   if ( ('4' == mode) || ('5' == mode) ) {
    typeinfo T;
    T.set('I', false);
    identlist* P = 0;
    char fname[8] = "getchar";
    char *f_name = fname;
    parse_data::startFunction(T, f_name, P);
    char cname[] = "c";
    f_name = cname;
    identlist *L = new identlist(T, f_name, false);
    char pname[8] = "putchar";
    f_name = pname;
    parse_data::startFunction(T, f_name, L);
    return 0;
  }

  cerr << "Mode " << mode << " not implemented yet.\n";
  return 8;
}

int main(int argc, const char** argv)
{
  /*
      Process arguments, if any
  */

  if (1 == argc) {  // No arguments
    return usage(0);
  }

  char mode = ' ';
  const char* outfile = 0;
  const char* infile = 0;
  for (int i=1; i<argc; i++) {
    if ('-' != argv[i][0]) {
      // Argument doesn't start with -, assume it is an input file

      if (infile) {
        cerr << "More than one input file specified\n\t(";
        cerr << infile << " and " << argv[i] << "), not implemented\n";
        return 1;
      }
      infile = argv[i];
      continue;
    }

    // Argument starts with -
    // Error and usage information for any bogus switches

    if (0 == argv[i][1]) return usage(argv[i]);   
    if (0 != argv[i][2]) return usage(argv[i]);

    // Switches that are implemented:

    switch (argv[i][1]) {

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
                if ((mode != ' ') && (mode != argv[i][1])) {
                  cerr << "More than one mode specified (-" << mode;
                  cerr << " and " << argv[i] << ")\n";
                  return 2;
                }
                mode = argv[i][1];
                continue;

      case 'o':
                if (0==argv[i+1]) {
                  cerr << "Missing argument for -o\n";
                  return 3;
                }
                if (outfile) {
                  cerr << "More than one output file specified (";
                  cerr << outfile << " and " << argv[i+1] << ")\n";
                  return 4;
                }
                outfile = argv[i+1];
                i++;  // will be incremented again in for loop
                continue;
    };

    // Still going?  Must be a bogus switch.
    return usage(argv[i]);
  }

  /*
    Do the appropriate thing for the requested mode
  */

  if (outfile) {
    ofstream fout(outfile);
    if (!fout) {
      cerr << "Couldn't open output file " << outfile << "\n";
      return 5;
    }
    return compile(mode, infile, fout);
  } else {
    return compile(mode, infile, std::cout);
  }

}
