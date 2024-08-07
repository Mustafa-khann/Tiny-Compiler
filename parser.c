#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

#define MAX_ERROR_LENGTH 1000

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = 1;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%s'", token->lexeme);

    }

    fprintf(stderr, ": %s\n", message);
    parser->hadError = 1;
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->previous, message);
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;

    for (;;) {
        parser->current = nextToken(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser, parser->current.lexeme);
    }
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

static int check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return 0;
    advance(parser);
    return 1;
}

static Node* createNode(NodeType type) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = type;
    return node;
}

static Node* expression(Parser* parser);
static Node* statement(Parser* parser);
static Node* declaration(Parser* parser);

static Node* primary(Parser* parser) {
    if (match(parser, TOKEN_INTEGER_LITERAL) || match(parser, TOKEN_FLOAT_LITERAL)) {
        Node* node = createNode(NODE_LITERAL);
        node->token = parser->previous;
        return node;
    }
    if (match(parser, TOKEN_IDENTIFIER)) {
        Node* node = createNode(NODE_IDENTIFIER);
        node->token = parser->previous;
        return node;
    }
    if (match(parser, TOKEN_LPAREN)) {
        Node* expr = expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }
    error(parser, "Expect expression.");
    return NULL;
}

static Node* unary(Parser* parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_BANG)) {
        Node* node = createNode(NODE_UNARY);
        node->token = parser->previous;
        node->as.unary.operand = unary(parser);
        return node;
    }

    return primary(parser);
}

static Node* factor(Parser* parser) {
    Node* node = unary(parser);

    while (match(parser, TOKEN_ASTERISK) || match(parser, TOKEN_SLASH)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = unary(parser);
        node = newNode;
    }

    return node;
}

static Node* term(Parser* parser) {
    Node* node = factor(parser);

    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = factor(parser);
        node = newNode;
    }

    return node;
}

static Node* comparison(Parser* parser) {
    Node* node = term(parser);

    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL) ||
           match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = term(parser);
        node = newNode;
    }

    return node;
}

static Node* equality(Parser* parser) {
    Node* node = comparison(parser);

    while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = comparison(parser);
        node = newNode;
    }

    return node;
}

static Node* logicalAnd(Parser* parser) {
    Node* node = equality(parser);

    while (match(parser, TOKEN_AND)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = equality(parser);
        node = newNode;
    }

    return node;
}

static Node* logicalOr(Parser* parser) {
    Node* node = logicalAnd(parser);

    while (match(parser, TOKEN_OR)) {
        Node* newNode = createNode(NODE_BINARY);
        newNode->token = parser->previous;
        newNode->as.binary.left = node;
        newNode->as.binary.right = logicalAnd(parser);
        node = newNode;
    }

    return node;
}

static Node* assignment(Parser* parser) {
    Node* node = logicalOr(parser);

    if (match(parser, TOKEN_ASSIGN)) {
        Node* newNode = createNode(NODE_ASSIGNMENT);
        newNode->token = parser->previous;
        newNode->as.assignment.left = node;
        newNode->as.assignment.right = assignment(parser);
        return newNode;
    }

    return node;
}

static Node* expression(Parser* parser) {
    return assignment(parser);
}

static Node* expressionStatement(Parser* parser) {
    Node* node = createNode(NODE_EXPRESSION_STATEMENT);
    node->as.expression_statement.expression = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    return node;
}

static Node* ifStatement(Parser* parser) {
    Node* node = createNode(NODE_IF_STATEMENT);
    consume(parser, TOKEN_LPAREN, "Expect '(' after 'if'.");
    node->as.if_statement.condition = expression(parser);
    consume(parser, TOKEN_RPAREN, "Expect ')' after if condition.");
    node->as.if_statement.then_branch = statement(parser);
    node->as.if_statement.else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        node->as.if_statement.else_branch = statement(parser);
    }
    return node;
}

static Node* whileStatement(Parser* parser) {
    Node* node = createNode(NODE_WHILE_STATEMENT);
    consume(parser, TOKEN_LPAREN, "Expect '(' after 'while'.");
    node->as.while_statement.condition = expression(parser);
    consume(parser, TOKEN_RPAREN, "Expect ')' after while condition.");
    node->as.while_statement.body = statement(parser);
    return node;
}

static Node* block(Parser* parser) {
    Node* node = createNode(NODE_BLOCK);
    node->as.block.statements = NULL;
    node->as.block.statement_count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Node* stmt = declaration(parser);
        node->as.block.statement_count++;
        node->as.block.statements = realloc(node->as.block.statements, node->as.block.statement_count * sizeof(Node*));
        node->as.block.statements[node->as.block.statement_count - 1] = stmt;
    }

    consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    return node;
}

static Node* returnStatement(Parser* parser) {
    Node* node = createNode(NODE_RETURN_STATEMENT);
    if (!check(parser, TOKEN_SEMICOLON)) {
        node->as.return_statement.expression = expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
    return node;
}

static Node* statement(Parser* parser) {
    if (match(parser, TOKEN_IF)) return ifStatement(parser);
    if (match(parser, TOKEN_WHILE)) return whileStatement(parser);
    if (match(parser, TOKEN_RETURN)) return returnStatement(parser);
    if (match(parser, TOKEN_LBRACE)) return block(parser);
    return expressionStatement(parser);
}

static Node* varDeclaration(Parser* parser) {
    Node* node = createNode(NODE_VARIABLE_DECLARATION);
    node->as.variable_declaration.type = createNode(NODE_TYPE);
    node->as.variable_declaration.type->token = parser->previous;

    consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
    node->as.variable_declaration.identifier = createNode(NODE_IDENTIFIER);
    node->as.variable_declaration.identifier->token = parser->previous;

    if (match(parser, TOKEN_ASSIGN)) {
        node->as.variable_declaration.initializer = expression(parser);
    } else {
        node->as.variable_declaration.initializer = NULL;
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    return node;
}

static Node* funDeclaration(Parser* parser) {
    Node* node = createNode(NODE_FUNCTION_DECLARATION);
    node->as.function_declaration.type = createNode(NODE_TYPE);
    node->as.function_declaration.type->token = parser->previous;

    consume(parser, TOKEN_IDENTIFIER, "Expect function name.");
    node->as.function_declaration.identifier = createNode(NODE_IDENTIFIER);
    node->as.function_declaration.identifier->token = parser->previous;

    consume(parser, TOKEN_LPAREN, "Expect '(' after function name.");
    node->as.function_declaration.parameters = NULL;
    node->as.function_declaration.parameter_count = 0;

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            if (node->as.function_declaration.parameter_count >= 255) {
                error(parser, "Can't have more than 255 parameters.");
            }
            node->as.function_declaration.parameter_count++;
            node->as.function_declaration.parameters = realloc(node->as.function_declaration.parameters,
                                                               node->as.function_declaration.parameter_count * sizeof(Node*));

            Node* param = createNode(NODE_VARIABLE_DECLARATION);
            if (match(parser, TOKEN_INT) || match(parser, TOKEN_FLOAT)) {
                param->as.variable_declaration.type = createNode(NODE_TYPE);
                param->as.variable_declaration.type->token = parser->previous;
            } else {
                error(parser, "Expect parameter type.");
            }

            consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
            param->as.variable_declaration.identifier = createNode(NODE_IDENTIFIER);
            param->as.variable_declaration.identifier->token = parser->previous;

            node->as.function_declaration.parameters[node->as.function_declaration.parameter_count - 1] = param;
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RPAREN, "Expect ')' after parameters.");
    consume(parser, TOKEN_LBRACE, "Expect '{' before function body.");
    node->as.function_declaration.body = block(parser);
    return node;
}

static Node* declaration(Parser* parser) {
    if (match(parser, TOKEN_INT) || match(parser, TOKEN_FLOAT)) {
        if (check(parser, TOKEN_IDENTIFIER) && parser->lexer->current[0] == '(') {
            return funDeclaration(parser);
        } else {
            return varDeclaration(parser);
        }
    }
    return statement(parser);
}

Node* parseProgram(Parser* parser) {
    Node* program = createNode(NODE_PROGRAM);
    program->as.program.declarations = NULL;
    program->as.program.declaration_count = 0;

    while (!match(parser, TOKEN_EOF)) {
        Node* decl = declaration(parser);
        if (decl) {
            program->as.program.declaration_count++;
            program->as.program.declarations = realloc(program->as.program.declarations,
                program->as.program.declaration_count * sizeof(Node*));
            program->as.program.declarations[program->as.program.declaration_count - 1] = decl;
        }
    }

    return program;
}

void freeAST(Node* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->as.program.declaration_count; i++) {
                freeAST(node->as.program.declarations[i]);
            }
            free(node->as.program.declarations);
            break;
        case NODE_FUNCTION_DECLARATION:
            freeAST(node->as.function_declaration.type);
            freeAST(node->as.function_declaration.identifier);
            for (int i = 0; i < node->as.function_declaration.parameter_count; i++) {
                freeAST(node->as.function_declaration.parameters[i]);
            }
            free(node->as.function_declaration.parameters);
            freeAST(node->as.function_declaration.body);
            break;
        case NODE_VARIABLE_DECLARATION:
            freeAST(node->as.variable_declaration.type);
            freeAST(node->as.variable_declaration.identifier);
            freeAST(node->as.variable_declaration.initializer);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->as.block.statement_count; i++) {
                freeAST(node->as.block.statements[i]);
            }
            free(node->as.block.statements);
            break;
        case NODE_IF_STATEMENT:
            freeAST(node->as.if_statement.condition);
            freeAST(node->as.if_statement.then_branch);
            freeAST(node->as.if_statement.else_branch);
            break;
        case NODE_WHILE_STATEMENT:
            freeAST(node->as.while_statement.condition);
            freeAST(node->as.while_statement.body);
            break;
        case NODE_RETURN_STATEMENT:
            freeAST(node->as.return_statement.expression);
            break;
        case NODE_EXPRESSION_STATEMENT:
            freeAST(node->as.expression_statement.expression);
            break;
        case NODE_BINARY:
            freeAST(node->as.binary.left);
            freeAST(node->as.binary.right);
            break;
        case NODE_UNARY:
            freeAST(node->as.unary.operand);
            break;
        case NODE_ASSIGNMENT:
            freeAST(node->as.assignment.left);
            freeAST(node->as.assignment.right);
            break;
        default:
            // For NODE_LITERAL, NODE_IDENTIFIER, and NODE_TYPE, no additional freeing is needed
            break;
    }

    free(node);
}

void initParser(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->hadError = 0;
    parser->panicMode = 0;
    advance(parser);
}
