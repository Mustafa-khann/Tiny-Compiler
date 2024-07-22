#include <cstddef>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "parser.h"
#include "token.h"

static Environment* createEnvironment(Environment* enclosing) {
    Environment* env = malloc(sizeof(Environment));
    env->variables = NULL;
    env->variable_count = 0;
    env->enclosing = enclosing;
    return env;
}

static void defineVariable(Environment* env, const char* name, Value value){
    env->variable_count++;
    env->variables = realloc(env->variables, env->variable_count * sizeof(Variable));
    env->variables[env->variable_count - 1].name = strdup(name);
    env->variables[env->variable_count - 1].value = value;
}

static Value* getVariable(Environment* env, const char* name){
    for(int i = 0; i<env->variable_count; i++) {
        if(strcmp(env->variables[i].name, name) == 0){
            return &env->variables[i].value;
        }
    }
    if(env->enclosing)
        {
            return getVariable(env->enclosing,name);
        }
    return NULL;
}

void initInterpreter(Interpreter *interpreter){
    interpreter->global_env = createEnvironment(NULL);
    interpreter->current_env = interpreter->global_env;
}

static Value evaluateExpression(Interpreter* interpreter, Node* node){
    switch (node->type)
    {
        case NODE_BINARY: {
            Value left = evaluateExpression(interpreter, node->as.binary.left);
            Value right = evaluateExpression(interpreter, node->as.binary.right);
            Value result = {VALUE_INT, {0}};

            if(left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
                result.type = VALUE_FLOAT;
                float left_val = (left.type == VALUE_FLOAT) ? left.as.float_value : (float)left.as.int_value;
                float right_val = (right.type == VALUE_FLOAT) ? right.as.float_value: (float)right.as.int_value;

                switch (node->token.type) {
                    case TOKEN_PLUS:
                    result.as.float_value = left_val + right_val;
                    break;
                    case TOKEN_MINUS:
                    result.as.float_value = left_val - right_val;
                    break;
                    case TOKEN_ASTERISK:
                    result.as.float_value = left_val * right_val;
                    case TOKEN_SLASH:
                    result.as.float_value = left_val / right_val;
                    break;
                    default:
                    fprintf(stderr, "Unknown operator for float operation\n");
                    exit(1);

                }

            }
            else {
                switch (node->token.type) {
                                    case TOKEN_PLUS:
                                        result.as.int_value = left.as.int_value + right.as.int_value;
                                        break;
                                    case TOKEN_MINUS:
                                        result.as.int_value = left.as.int_value - right.as.int_value;
                                        break;
                                    case TOKEN_ASTERISK:
                                        result.as.int_value = left.as.int_value * right.as.int_value;
                                        break;
                                    case TOKEN_SLASH:
                                        result.as.int_value = left.as.int_value / right.as.int_value;
                                        break;
                                    // Add other operators as needed
                                    default:
                                        fprintf(stderr, "Unknown operator for int operation\n");
                                        exit(1);
                                }
            }



        }
    }
}
