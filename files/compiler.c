#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "scanner.h"

//Current parser state
typedef struct
{
    Token current;
    Token previous;
    int hadError;
    int panicMode;
} Parser;

Parser parser;
Chunk* compilingChunk;

//Get current chunk
static Chunk* currentChunk()
{
    return compilingChunk;
}

//Report error
static void errorAt(Token* token, const char* message)
{
    if (parser.panicMode)
    {
        return;
    }

    parser.panicMode = 1;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = 1;
}

//Error at previous token
static void error(const char* message)
{
    errorAt(&parser.previous, message);
}

//Error at current token
static void errorAtCurrent(const char* message)
{
    errorAt(&parser.current, message);
}

//Advance token
static void advanceParser()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();

        if (parser.current.type != TOKEN_ERROR)
        {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

//Consume required token
static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type)
    {
        advanceParser();
        return;
    }

    errorAtCurrent(message);
}

//Emit byte
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte);
}

//Emit two bytes
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

//Emit return
static void emitReturn()
{
    emitByte(OP_RETURN);
}

//Add constant
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);

    if (constant > 255)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

//Emit constant
static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

//Forward declarations
static void expression();
static void statement();
static void declaration();

//Compile primary expression
static void primary()
{
    switch (parser.current.type)
    {
        case TOKEN_NUMBER:
        {
            advanceParser();
            double value = strtod(parser.previous.start, NULL);
            emitConstant(value);
            return;
        }

        case TOKEN_LEFT_PAREN:
        {
            advanceParser();
            expression();
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
            return;
        }

        default:
            errorAtCurrent("Expect expression.");
            advanceParser();
            return;
    }
}

//Compile unary expression
static void unary()
{
    if (parser.current.type == TOKEN_MINUS)
    {
        advanceParser();
        unary();
        emitByte(OP_NEGATE);
        return;
    }

    primary();
}

//Compile factor expression
static void factor()
{
    unary();

    while (parser.current.type == TOKEN_STAR ||
           parser.current.type == TOKEN_SLASH)
    {
        TokenType operatorType = parser.current.type;
        advanceParser();

        unary();

        switch (operatorType)
        {
            case TOKEN_STAR:
                emitByte(OP_MULTIPLY);
                break;

            case TOKEN_SLASH:
                emitByte(OP_DIVIDE);
                break;

            default:
                return;
        }
    }
}

//Compile term expression
static void term()
{
    factor();

    while (parser.current.type == TOKEN_PLUS ||
           parser.current.type == TOKEN_MINUS)
    {
        TokenType operatorType = parser.current.type;
        advanceParser();

        factor();

        switch (operatorType)
        {
            case TOKEN_PLUS:
                emitByte(OP_ADD);
                break;

            case TOKEN_MINUS:
                emitByte(OP_SUBTRACT);
                break;

            default:
                return;
        }
    }
}

//Compile expression
static void expression()
{
    term();
}

//Compile print statement
static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

//Compile statement
static void statement()
{
    if (parser.current.type == TOKEN_PRINT)
    {
        advanceParser();
        printStatement();
    }
    else
    {
        errorAtCurrent("Expect statement.");
        advanceParser();
    }
}

//Compile declaration
static void declaration()
{
    statement();
}

//Compile source
int compile(const char* source, Chunk* chunk)
{
    initScanner(source);

    compilingChunk = chunk;

    parser.hadError = 0;
    parser.panicMode = 0;

    advanceParser();

    while (parser.current.type != TOKEN_EOF)
    {
        declaration();
    }

    emitReturn();

    return !parser.hadError;
}