#include <string.h>
#include "scanner.h"

//Current scanner state
typedef struct
{
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

//Initialize scanner with source
void initScanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

//Check end of source
static int isAtEnd()
{
    return *scanner.current == '\0';
}

//Create token
static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

//Create error token
static Token errorToken(const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

//Advance one character
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

//Peek current character
static char peek()
{
    return *scanner.current;
}

//Check digit
static int isDigit(char c)
{
    return c >= '0' && c <= '9';
}

//Check alphabetic
static int isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

//Skip whitespace
static void skipWhitespace()
{
    for (;;)
    {
        char c = peek();

        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                scanner.line++;
                advance();
                break;

            case '/':
                if (scanner.current[1] == '/')
                {
                    while (peek() != '\n' && !isAtEnd())
                    {
                        advance();
                    }
                }
                else
                {
                    return;
                }
                break;

            default:
                return;
        }
    }
}

//Scan number
static Token number()
{
    while (isDigit(peek()))
    {
        advance();
    }

    if (peek() == '.' && isDigit(scanner.current[1]))
    {
        advance();

        while (isDigit(peek()))
        {
            advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

//Check keyword text
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_ERROR;
}

//Get identifier type
static TokenType identifierType()
{
    switch (scanner.start[0])
    {
        case 'p':
            return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    }

    return TOKEN_ERROR;
}

//Scan identifier
static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
    {
        advance();
    }

    TokenType type = identifierType();

    if (type == TOKEN_ERROR)
    {
        return errorToken("Unexpected identifier.");
    }

    return makeToken(type);
}

//Scan next token
Token scanToken()
{
    skipWhitespace();

    scanner.start = scanner.current;

    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();

    if (isDigit(c))
    {
        return number();
    }

    if (isAlpha(c))
    {
        return identifier();
    }

    switch (c)
    {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);

        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);

        case '-':
            return makeToken(TOKEN_MINUS);

        case '+':
            return makeToken(TOKEN_PLUS);

        case '/':
            return makeToken(TOKEN_SLASH);

        case '*':
            return makeToken(TOKEN_STAR);

        case ';':
            return makeToken(TOKEN_SEMICOLON);
    }

    return errorToken("Unexpected character.");
}