#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Node types
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DECLARATION,
    NODE_VARIABLE_DECLARATION,
    NODE_TYPE,
    NODE_IDENTIFIER,
    NODE_BLOCK,
    NODE_IF_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_EXPRESSION_STATEMENT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_PRIMARY,
    NODE_ASSIGNMENT,
    NODE_LITERAL
} NodeType;

// Forward declaration of Node
typedef struct Node Node;

// Node structure
struct Node {
    NodeType type;
    Token token;
    union {
        struct {
            Node** declarations;
            int declaration_count;
        } program;
        struct {
            Node* type;
            Node* identifier;
            Node** parameters;
            int parameter_count;
            Node* body;
        } function_declaration;
        struct {
            Node* type;
            Node* identifier;
            Node* initializer;
        } variable_declaration;
        struct {
            Node** statements;
            int statement_count;
        } block;
        struct {
            Node* condition;
            Node* then_branch;
            Node* else_branch;
        } if_statement;
        struct {
            Node* condition;
            Node* body;
        } while_statement;
        struct {
            Node* expression;
        } return_statement;
        struct {
            Node* expression;
        } expression_statement;
        struct {
            Node* left;
            Node* right;
        } binary;
        struct {
            Node* operand;
        } unary;
        struct {
            Token value;
        } literal;
        struct {
            Node* left;
            Node* right;
        } assignment;
    } as;
};

// Parser structure
typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    int hadError;
    int panicMode;
} Parser;

// Function prototypes
void initParser(Parser* parser, Lexer* lexer);
Node* parseProgram(Parser* parser);

#endif // PARSER_H
