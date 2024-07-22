#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "token.h"

void initLexer(Lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static int isAtEnd(Lexer *lexer)
{
    return *lexer->current == '\0';

}

static char advance(Lexer *lexer)
{
    lexer->current++;
    return lexer->current[-1];
}

static int match(Lexer *lexer, char expected)
{
    if(isAtEnd(lexer))
        {
            return 0;
        }
    if(*lexer->current != expected) return 0;
    lexer->current++;
    return 1;
}

static Token makeToken(Lexer *lexer, TokenType type)
{
    Token token;
    token.type = type;
    token.lexeme = lexer->start;
    token.line = lexer->line;
    return token;
}

static Token errorToken(Lexer *lexer, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.lexeme = message;
    token.line = lexer->line;
    return token;
}

// function to skip spaces and comments
static void skipwhitespace(Lexer *lexer)
{
    for(;;)
        {
            char c = *lexer->current;
            switch(c)
            {
                case ' ':
                case '\r':
                case '\t':
                    advance(lexer);
                    break;
                case '\n':
                    lexer->line++;
                    advance(lexer);
                    break;
                case '/':
                    if(match(lexer, '/'))
                        {
                            while(*lexer->current != '/' && isAtEnd(lexer)) advance(lexer);
                        }
                    else {
                        return;
                    }
                    break;
                  default:
                    return;
            }

        }
}

// Function to recognize a keyword and return the correct token type
static TokenType checkKeyword(Lexer *lexer, int start, int length, const char *rest, TokenType type){
    if(strncmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// This function identifies if the current lexeme is a keyword or identifier
static TokenType identifierType(Lexer *lexer)
{
    switch(lexer->start[0]) {
        case 'i':
            if(lexer->start - lexer->current > 1) {
                switch (lexer->start[1]) {
                    case 'f': return checkKeyword(lexer, 2, 0, "", TOKEN_IF);
                    case 'n': return checkKeyword(lexer, 2, 1, "t", TOKEN_INT);
                }
            }
        break;
        case 'f':
            if(lexer->start - lexer->current > 1) {
                switch(lexer->start[1]) {
                    case 'l': return checkKeyword(lexer, 2, 3, "oat", TOKEN_FLOAT);
                }
            }
            break;
        case 'r': return checkKeyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
        case 'w': return checkKeyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

// Function to handle identifier and keywords
static Token identifier(Lexer *lexer){
    while (isalnum(*lexer->current) || *lexer->current == '_') advance(lexer);
    return makeToken(lexer, identifierType(lexer));
}

// Functions to handle numbers and floats
static Token number(Lexer *lexer)
{
    while(isdigit(*lexer->current)) advance(lexer);

    if(*lexer->current == '.' && isdigit(lexer->current[1]))
        {
            advance(lexer);
            while(isdigit(*lexer->current)) advance(lexer);
            return makeToken(lexer, TOKEN_FLOAT_LITERAL);
        }
    return makeToken(lexer, TOKEN_INTEGER_LITERAL);
}

// The main function to produce the next token from the input
Token nextToken(Lexer *lexer)
{
    skipwhitespace(lexer);
    lexer->start = lexer->current;

    if(isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

    char c = advance(lexer);

    if(isalpha(c)) return identifier(lexer);
    if(isdigit(c)) return number(lexer);

    switch(c) {
        case '+': return makeToken(lexer, TOKEN_PLUS);
        case '-': return makeToken(lexer, TOKEN_MINUS);
        case '*': return makeToken(lexer, TOKEN_ASTERISK);
        case '/': return makeToken(lexer, TOKEN_SLASH);
        case '=': return makeToken(lexer, TOKEN_ASSIGN);
        case ';': return makeToken(lexer, TOKEN_SEMICOLON);
        case ',': return makeToken(lexer, TOKEN_COMMA);
        case '(': return makeToken(lexer, TOKEN_LPAREN);
        case ')': return makeToken(lexer, TOKEN_RPAREN);
        case '{': return makeToken(lexer, TOKEN_LBRACE);
        case '}': return makeToken(lexer, TOKEN_RBRACE);
    }
    return errorToken(lexer, "Unexpected Character");
}
