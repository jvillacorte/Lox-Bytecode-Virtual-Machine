#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include <stdint.h>
#include "value.h"

//Bytecode instructions
typedef enum
{
    OP_CONSTANT,

    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    OP_NOT,
    OP_NEGATE,

    OP_AND,
    OP_OR,

    OP_PRINT,
    OP_POP,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    
    OP_RETURN
} OpCode;

//Chunk holds bytecode and constants
typedef struct
{
    int count;
    int capacity;
    uint8_t* code;

    int constantsCount;
    int constantsCapacity;
    Value* constants;

} Chunk;

//Initialize chunk
void initChunk(Chunk* chunk);

//Write byte to chunk
void writeChunk(Chunk* chunk, uint8_t byte);

//Add constant to pool
int addConstant(Chunk* chunk, Value value);

//Free memory
void freeChunk(Chunk* chunk);

#endif