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

static void defineVariable(Environment* env, const char* name, Value value) {
    env->variable_count++;
    env->variables = realloc(env->variables, env->variable_count * sizeof(Variable));
    env->variables[env->variable_count - 1].name = strdup(name);
    env->variables[env->variable_count - 1].value = value;
}

static Value* getVariable(Environment* env, const char* name) {
    for (int i = 0; i < env->variable_count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            return &env->variables[i].value;
        }
    }
    if (env->enclosing) {
        return getVariable(env->enclosing, name);
    }
    return NULL;
}

void initInterpreter(Interpreter* interpreter) {
    interpreter->global_env = createEnvironment(NULL);
    interpreter->current_env = interpreter->global_env;
}

static Value evaluateExpression(Interpreter* interpreter, Node* node) {
    switch (node->type) {
        case NODE_BINARY: {
            Value left = evaluateExpression(interpreter, node->as.binary.left);
            Value right = evaluateExpression(interpreter, node->as.binary.right);
            Value result = {VALUE_INT, {0}};

            if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
                result.type = VALUE_FLOAT;
                float left_val = (left.type == VALUE_FLOAT) ? left.as.float_value : (float)left.as.int_value;
                float right_val = (right.type == VALUE_FLOAT) ? right.as.float_value : (float)right.as.int_value;

                switch (node->token.type) {
                    case TOKEN_PLUS:
                        result.as.float_value = left_val + right_val;
                        break;
                    case TOKEN_MINUS:
                        result.as.float_value = left_val - right_val;
                        break;
                    case TOKEN_ASTERISK:
                        result.as.float_value = left_val * right_val;
                        break;
                    case TOKEN_SLASH:
                        result.as.float_value = left_val / right_val;
                        break;
                    default:
                        fprintf(stderr, "Unknown operator for float operation\n");
                        exit(1);
                }
            } else {
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
                    default:
                        fprintf(stderr, "Unknown operator for int operation\n");
                        exit(1);
                }
            }
            return result;
        }
        case NODE_UNARY: {
            Value operand = evaluateExpression(interpreter, node->as.unary.operand);
            Value result = {operand.type, {0}};

            switch (node->token.type) {
                case TOKEN_MINUS:
                    if (operand.type == VALUE_INT) {
                        result.as.int_value = -operand.as.int_value;
                    } else {
                        result.as.float_value = -operand.as.float_value;
                    }
                    break;
                default:
                    fprintf(stderr, "Unknown unary operator\n");
                    exit(1);
            }
            return result;
        }
        case NODE_LITERAL: {
            Value result;
            if (node->token.type == TOKEN_INTEGER_LITERAL) {
                result.type = VALUE_INT;
                result.as.int_value = atoi(node->token.lexeme);
            } else if (node->token.type == TOKEN_FLOAT_LITERAL) {
                result.type = VALUE_FLOAT;
                result.as.float_value = atof(node->token.lexeme);
            }
            return result;
        }
        case NODE_IDENTIFIER: {
            Value* value = getVariable(interpreter->current_env, node->token.lexeme);
            if (value == NULL) {
                fprintf(stderr, "Undefined variable: %s\n", node->token.lexeme);
                exit(1);
            }
            return *value;
        }
        case NODE_ASSIGNMENT: {
            Value value = evaluateExpression(interpreter, node->as.assignment.right);
            Value* variable = getVariable(interpreter->current_env, node->as.assignment.left->token.lexeme);
            if (variable == NULL) {
                fprintf(stderr, "Undefined variable: %s\n", node->as.assignment.left->token.lexeme);
                exit(1);
            }
            *variable = value;
            return value;
        }
        default:
            fprintf(stderr, "Unknown node type in expression\n");
            exit(1);
    }
}

static void executeStatement(Interpreter* interpreter, Node* node) {
    switch (node->type) {
        case NODE_EXPRESSION_STATEMENT: {
            evaluateExpression(interpreter, node->as.expression_statement.expression);
            break;
        }
        case NODE_VARIABLE_DECLARATION: {
            Value value = {VALUE_INT, {0}};  // Default initialization
            if (node->as.variable_declaration.type->token.type == TOKEN_FLOAT) {
                value.type = VALUE_FLOAT;
            }
            if (node->as.variable_declaration.initializer != NULL) {
                value = evaluateExpression(interpreter, node->as.variable_declaration.initializer);
            }
            defineVariable(interpreter->current_env, node->as.variable_declaration.identifier->token.lexeme, value);
            break;
        }
        case NODE_IF_STATEMENT: {
            Value condition = evaluateExpression(interpreter, node->as.if_statement.condition);
            if (condition.type == VALUE_INT ? condition.as.int_value : condition.as.float_value) {
                executeStatement(interpreter, node->as.if_statement.then_branch);
            } else if (node->as.if_statement.else_branch != NULL) {
                executeStatement(interpreter, node->as.if_statement.else_branch);
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            while (1) {
                Value condition = evaluateExpression(interpreter, node->as.while_statement.condition);
                if (!(condition.type == VALUE_INT ? condition.as.int_value : condition.as.float_value)) {
                    break;
                }
                executeStatement(interpreter, node->as.while_statement.body);
            }
            break;
        }
        case NODE_BLOCK: {
            Environment* previous = interpreter->current_env;
            interpreter->current_env = createEnvironment(previous);
            for (int i = 0; i < node->as.block.statement_count; i++) {
                executeStatement(interpreter, node->as.block.statements[i]);
            }
            interpreter->current_env = previous;
            break;
        }
        case NODE_RETURN_STATEMENT: {
            // This will be handled in evaluateNode
            break;
        }
        default:
            fprintf(stderr, "Unknown statement type\n");
            exit(1);
    }
}

Value evaluateNode(Interpreter* interpreter, Node* node) {
    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->as.program.declaration_count; i++) {
                evaluateNode(interpreter, node->as.program.declarations[i]);
            }
            return (Value){VALUE_VOID, {0}};
        case NODE_FUNCTION_DECLARATION:
            // Handle function declaration (if implemented)
            return (Value){VALUE_VOID, {0}};
        case NODE_VARIABLE_DECLARATION:
        case NODE_IF_STATEMENT:
        case NODE_WHILE_STATEMENT:
        case NODE_BLOCK:
        case NODE_EXPRESSION_STATEMENT:
            executeStatement(interpreter, node);
            return (Value){VALUE_VOID, {0}};
        case NODE_RETURN_STATEMENT:
            if (node->as.return_statement.expression != NULL) {
                return evaluateExpression(interpreter, node->as.return_statement.expression);
            } else {
                return (Value){VALUE_VOID, {0}};
            }
        case NODE_BINARY:
        case NODE_UNARY:
        case NODE_LITERAL:
        case NODE_IDENTIFIER:
        case NODE_ASSIGNMENT:
            return evaluateExpression(interpreter, node);
        default:
            fprintf(stderr, "Unknown node type in evaluateNode\n");
            exit(1);
    }
}

void interpret(Interpreter* interpreter, Node* program) {
    Value result = evaluateNode(interpreter, program);
    printValue(result);
}

void printValue(Value value) {
    switch(value.type) {
        case VALUE_INT:
            printf("%d\n", value.as.int_value);
            break;
        case VALUE_FLOAT:
            printf("%f\n", value.as.float_value);
            break;
        case VALUE_VOID:
            printf("void\n");
            break;
    }
}
