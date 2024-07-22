#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_VOID
} ValueType;

typedef struct {
    ValueType type;
    union {
        int int_value;
        float float_value;
    } as ;
} Value;

typedef struct {
    char* name;
    Value value;
} Variable;

typedef struct Environment {
    Variable* variables;
    int variable_count;
    struct Environment* enclosing;
}Environment;

typedef struct {
    Environment* global_env;
    Environment* current_env;
}Interpreter;

void initInterpreter(Interpreter* interpreter);
void interpret(Interpreter* interpreter, Node* program);
Value evaluateNode(Interpreter* interpreter, Node* node);

#endif
