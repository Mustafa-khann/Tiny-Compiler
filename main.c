#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <script>\n", argv[0]);
        exit(64);
    }
    char* source = readFile(argv[1]);
    Lexer lexer;
    initLexer(&lexer, source);
    Parser parser;
    initParser(&parser, &lexer);
    Node* program = parseProgram(&parser);
    Interpreter interpreter;
    initInterpreter(&interpreter);
    interpret(&interpreter, program);

    // Free allocated memory (implement proper cleanup functions)
    free(source);
    // TODO: Implement and call functions to free the AST and other allocated structures
    return 0;
}
