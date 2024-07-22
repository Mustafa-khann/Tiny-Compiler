# TinyCompiler

TinyCompiler is a simple compiler implementation written in C. It includes a lexer, parser, and interpreter for a basic programming language.

## Features

- Lexical analysis
- Parsing
- Abstract Syntax Tree (AST) generation
- Basic interpretation

## Getting Started

### Prerequisites

- GCC or any C compiler
- Make (optional, for building using the provided Makefile)

### Building

1. Clone the repository:
   ```
   git clone https://github.com/mustafa-khann/tiny-compiler.git
   cd tinycompiler
   ```

2. Compile the project:
   ```
   make
   ```
   Or manually:
   ```
   gcc -o tinycompiler main.c lexer.c parser.c interpreter.c -I.
   ```

### Running

Run the compiler on a source file:

```
./tinycompiler script.tc
```

## Project Structure

- `token.h`: Defines token types and structure
- `lexer.h` / `lexer.c`: Lexical analyzer implementation
- `parser.h` / `parser.c`: Parser implementation
- `interpreter.h` / `interpreter.c`: Interpreter implementation
- `main.c`: Main program
