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

//Make identifier constant
static uint8_t identifierConstant(Token* name)
{
    return makeConstant(stringValue(name->start, name->length));
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
        case TOKEN_FALSE:
            advanceParser();
            emitByte(OP_FALSE);
            return;

        case TOKEN_TRUE:
            advanceParser();
            emitByte(OP_TRUE);
            return;

        case TOKEN_NIL:
            advanceParser();
            emitByte(OP_NIL);
            return;

        case TOKEN_NUMBER:
        {
            advanceParser();
            double value = strtod(parser.previous.start, NULL);
            emitConstant(numberValue(value));
            return;
        }

        case TOKEN_IDENTIFIER:
        {
            advanceParser();
            uint8_t name = identifierConstant(&parser.previous);
            emitBytes(OP_GET_GLOBAL, name);
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
    if (parser.current.type == TOKEN_BANG)
    {
        advanceParser();
        unary();
        emitByte(OP_NOT);
        return;
    }

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

//Compile comparison expression
static void comparison()
{
    term();

    while (parser.current.type == TOKEN_GREATER ||
           parser.current.type == TOKEN_GREATER_EQUAL ||
           parser.current.type == TOKEN_LESS ||
           parser.current.type == TOKEN_LESS_EQUAL)
    {
        TokenType operatorType = parser.current.type;
        advanceParser();

        term();

        switch (operatorType)
        {
            case TOKEN_GREATER:
                emitByte(OP_GREATER);
                break;

            case TOKEN_GREATER_EQUAL:
                emitByte(OP_LESS);
                emitByte(OP_NOT);
                break;

            case TOKEN_LESS:
                emitByte(OP_LESS);
                break;

            case TOKEN_LESS_EQUAL:
                emitByte(OP_GREATER);
                emitByte(OP_NOT);
                break;

            default:
                return;
        }
    }
}

//Compile equality expression
static void equality()
{
    comparison();

    while (parser.current.type == TOKEN_BANG_EQUAL ||
           parser.current.type == TOKEN_EQUAL_EQUAL)
    {
        TokenType operatorType = parser.current.type;
        advanceParser();

        comparison();

        switch (operatorType)
        {
            case TOKEN_BANG_EQUAL:
                emitByte(OP_EQUAL);
                emitByte(OP_NOT);
                break;

            case TOKEN_EQUAL_EQUAL:
                emitByte(OP_EQUAL);
                break;

            default:
                return;
        }
    }
}

//Compile and expression
static void logicAnd()
{
    equality();

    while (parser.current.type == TOKEN_AND)
    {
        advanceParser();
        equality();
        emitByte(OP_AND);
    }
}

//Compile or expression
static void logicOr()
{
    logicAnd();

    while (parser.current.type == TOKEN_OR)
    {
        advanceParser();
        logicAnd();
        emitByte(OP_OR);
    }
}

//Compile expression
static void expression()
{
    logicOr();
}

//Compile variable declaration
static void varDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect variable name.");

    Token nameToken = parser.previous;
    uint8_t global = identifierConstant(&nameToken);

    if (parser.current.type == TOKEN_EQUAL)
    {
        advanceParser();
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    emitBytes(OP_DEFINE_GLOBAL, global);
}

//Compile print statement
static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

//Compile assignment statement
static void assignmentStatement()
{
    Token nameToken = parser.current;
    advanceParser();

    consume(TOKEN_EQUAL, "Expect '=' after variable name.");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after assignment.");

    uint8_t global = identifierConstant(&nameToken);
    emitBytes(OP_SET_GLOBAL, global);
}

//Compile statement
static void statement()
{
    if (parser.current.type == TOKEN_PRINT)
    {
        advanceParser();
        printStatement();
    }
    else if (parser.current.type == TOKEN_IDENTIFIER)
    {
        assignmentStatement();
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
    if (parser.current.type == TOKEN_VAR)
    {
        advanceParser();
        varDeclaration();
    }
    else
    {
        statement();
    }

    parser.panicMode = 0;
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