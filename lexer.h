#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
    int column;
} Lexer;

void initLexer(Lexer *lexer, const char *source);
Token nextToken(Lexer *lexer);

#endif
