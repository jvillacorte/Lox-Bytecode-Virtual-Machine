#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    int debugMode = 0;

    //Check debug flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        debugMode = 1;
    }

    Chunk chunk;
    initChunk(&chunk);

    //Load constants
    int constantA = addConstant(&chunk, 15.5);
    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, constantA);

    int constantB = addConstant(&chunk, 10);
    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, constantB);

    //Add and print
    writeChunk(&chunk, OP_ADD);
    writeChunk(&chunk, OP_PRINT);
    writeChunk(&chunk, OP_RETURN);

    if (debugMode)
    {
        disassembleChunk(&chunk, "test.lox");
    }
    else
    {
        VM vm;
        initVM(&vm);
        interpret(&vm, &chunk);
        freeVM(&vm);
    }

    freeChunk(&chunk);
    return 0;
}