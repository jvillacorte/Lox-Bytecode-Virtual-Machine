#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

//Token types
typedef enum
{
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,

    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_SEMICOLON,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,

    TOKEN_AND,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_TRUE,
    TOKEN_VAR,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

//Token structure
typedef struct
{
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

//Initialize scanner
void initScanner(const char* source);

//Get next token
Token scanToken();

#endif