#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

//Token types
typedef enum
{
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_SEMICOLON,

    TOKEN_NUMBER,
    TOKEN_PRINT,

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