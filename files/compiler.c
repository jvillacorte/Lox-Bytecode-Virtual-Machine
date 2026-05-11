#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "scanner.h"

#define UINT8_COUNT (UINT8_MAX + 1)

//Current parser state
typedef struct
{
    Token current;
    Token previous;
    int hadError;
    int panicMode;
} Parser;

//Local variable record
typedef struct
{
    Token name;
    int depth;
} Local;

//Compiler state for local scopes
typedef struct
{
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler compiler;
Chunk* compilingChunk;
VM* compilerVM;

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

//Begin scope
static void beginScope()
{
    compiler.scopeDepth++;
}

//End scope
static void endScope()
{
    compiler.scopeDepth--;

    while (compiler.localCount > 0 &&
           compiler.locals[compiler.localCount - 1].depth > compiler.scopeDepth)
    {
        emitByte(OP_POP);
        compiler.localCount--;
    }
}

//Compare identifiers
static int identifiersEqual(Token* a, Token* b)
{
    if (a->length != b->length)
    {
        return 0;
    }

    return memcmp(a->start, b->start, a->length) == 0;
}

//Add local variable
static void addLocal(Token name)
{
    if (compiler.localCount == UINT8_COUNT)
    {
        error("Too many local variables.");
        return;
    }

    Local* local = &compiler.locals[compiler.localCount++];
    local->name = name;
    local->depth = compiler.scopeDepth;
}

//Declare local variable
static void declareVariable(Token name)
{
    if (compiler.scopeDepth == 0)
    {
        return;
    }

    for (int i = compiler.localCount - 1; i >= 0; i--)
    {
        Local* local = &compiler.locals[i];

        if (local->depth != -1 && local->depth < compiler.scopeDepth)
        {
            break;
        }

        if (identifiersEqual(&name, &local->name))
        {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(name);
}

//Resolve local variable
static int resolveLocal(Token* name)
{
    for (int i = compiler.localCount - 1; i >= 0; i--)
    {
        Local* local = &compiler.locals[i];

        if (identifiersEqual(name, &local->name))
        {
            return i;
        }
    }

    return -1;
}

//Emit jump instruction
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);

    return currentChunk()->count - 2;
}

//Patch jump location
static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;

    if (jump > 65535)
    {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

//Emit loop jump
static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;

    if (offset > 65535)
    {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
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
    ObjString* string = internString(compilerVM, name->start, name->length);
    return makeConstant(stringValue(string));
}

//Forward declarations
static void expression();
static void statement();
static void declaration();
static void block();
static void assignment();

//Emit named variable access
static void namedVariable(Token name, int canAssign)
{
    uint8_t getOp;
    uint8_t setOp;
    int arg = resolveLocal(&name);

    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && parser.current.type == TOKEN_EQUAL)
    {
        advanceParser();
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        emitBytes(getOp, (uint8_t)arg);
    }
}

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

        case TOKEN_STRING:
        {
            advanceParser();

            ObjString* string = internString(
                compilerVM,
                parser.previous.start + 1,
                parser.previous.length - 2
            );

            emitConstant(stringValue(string));
            return;
        }

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
            namedVariable(parser.previous, 1);
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

//Compile and expression with short-circuiting
static void logicAnd()
{
    equality();

    while (parser.current.type == TOKEN_AND)
    {
        advanceParser();

        int endJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);

        equality();

        patchJump(endJump);
    }
}

//Compile or expression with short-circuiting
static void logicOr()
{
    logicAnd();

    while (parser.current.type == TOKEN_OR)
    {
        advanceParser();

        int elseJump = emitJump(OP_JUMP_IF_FALSE);
        int endJump = emitJump(OP_JUMP);

        patchJump(elseJump);
        emitByte(OP_POP);

        logicAnd();

        patchJump(endJump);
    }
}

//Compile assignment expression
static void assignment()
{
    logicOr();
}

//Compile expression
static void expression()
{
    assignment();
}

//Compile expression statement
static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

//Compile variable declaration
static void varDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect variable name.");

    Token name = parser.previous;

    if (compiler.scopeDepth > 0)
    {
        declareVariable(name);
    }

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

    if (compiler.scopeDepth == 0)
    {
        uint8_t global = identifierConstant(&name);
        emitBytes(OP_DEFINE_GLOBAL, global);
    }
}

//Compile print statement
static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

//Compile block
static void block()
{
    while (parser.current.type != TOKEN_RIGHT_BRACE &&
           parser.current.type != TOKEN_EOF)
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

//Compile if statement
static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (parser.current.type == TOKEN_ELSE)
    {
        advanceParser();
        statement();
    }

    patchJump(elseJump);
}

//Compile while statement
static void whileStatement()
{
    int loopStart = currentChunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    statement();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

//Compile statement
static void statement()
{
    if (parser.current.type == TOKEN_PRINT)
    {
        advanceParser();
        printStatement();
    }
    else if (parser.current.type == TOKEN_IF)
    {
        advanceParser();
        ifStatement();
    }
    else if (parser.current.type == TOKEN_WHILE)
    {
        advanceParser();
        whileStatement();
    }
    else if (parser.current.type == TOKEN_LEFT_BRACE)
    {
        advanceParser();
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
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
int compile(const char* source, Chunk* chunk, VM* vm)
{
    initScanner(source);

    compilingChunk = chunk;
    compilerVM = vm;
    compiler.localCount = 0;
    compiler.scopeDepth = 0;

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
