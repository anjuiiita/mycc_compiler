
# mycc compiler

This is the repository for building a Java compiler.

## To build the compiler please run following command:

make

## To run the comiler please run below command:

mycc

## Feature Part 0

For part 0,  mode 0 is implemented which is reading from file and printing it.

And generating developers.pdf file from developers.tex is implemented

### To run mycc in mode 0, give below command after mycc.

mycc -0

## Feature Part 1

For part 1,  mode 1 is implemented which uses flex to tokenize the input file. To run mode 1 please run below command.

mycc -1 <input_file>

For processing multiple files, run below command:

mycc -1 <input_file>  <input_file>... 

In Part 1,  #include is implemented.
If there is error opening include file, error is genrated and if a cycle if detected then also Error is generated at max depth 256.

#define is also implemented, if the defined var is a keyword, it generates approriate erroe message and also checks the length of replacement text and appropriate error is generated.

#ifdef, #ifndef #endif #else directive mismatched in implemented

#ifdef indetifier if not defined then Error is printed.

## Feature Part 2
For part 2,  mode 2 is implemented which uses lex and yacc to tokenize the input file and build parse tree. To run mode 2 please run below command.

mycc -2 <input_file>

Part 1 and 0 are working as before.

In part 2, it checks only the input file syntax.
Production rules are defined to check the syntax
EXTRA : variable initialization, CONSTANTS, STRUCT, struct member selection are defined.

## Feature Part 3
For part 3,  mode 3 is implemented which uses lex and yacc to tokenize the input file and build parse tree.
Did semantic analysis using symbol tables.

To run mode 2 please run below command.

mycc -3 <input_file>

Part 0, 1 and 2 are working as before.

In part 3, it does only the input file semantic analysis.
Production rules are defined to check the syntax
EXTRA :  initialization, const, user-defined structs, struct member selection are defined.

## Feature Part 4
For part 4,  mode 4 is implemented which stack machine to generate java bytecode using stack machin code.

To run mode 4 please run below command.

mycc -4 <input_file>

Part 0, 1, 2 and 3 are working as before.

In part 4, it generated the intermediate code for given input program
Arrays, Expressions, Function calls, user functions and always present output, operators
special methos clinit are implemented.
EXTRA :  initialization, handling global variables.


## To read from input file please run below command

mycc -o out.txt

## Command to clean the auto generated files

make clean