#include <stdlib.h>
#include "chunk.h"

//Grow array capacity
static int growCapacity(int capacity)
{
    return capacity < 8 ? 8 : capacity * 2;
}

//Reallocate memory
static void* growArray(void* pointer, size_t oldSize, size_t newSize)
{
    (void)oldSize;

    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    return realloc(pointer, newSize);
}

//Initialize chunk fields
void initChunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    chunk->constantsCount = 0;
    chunk->constantsCapacity = 0;
    chunk->constants = NULL;
}

//Write byte into code array
void writeChunk(Chunk* chunk, uint8_t byte)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = growCapacity(oldCapacity);

        chunk->code = growArray(
            chunk->code,
            sizeof(uint8_t) * oldCapacity,
            sizeof(uint8_t) * chunk->capacity
        );
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

//Add value to constant pool
int addConstant(Chunk* chunk, Value value)
{
    if (chunk->constantsCapacity < chunk->constantsCount + 1)
    {
        int oldCapacity = chunk->constantsCapacity;
        chunk->constantsCapacity = growCapacity(oldCapacity);

        chunk->constants = growArray(
            chunk->constants,
            sizeof(Value) * oldCapacity,
            sizeof(Value) * chunk->constantsCapacity
        );
    }

    chunk->constants[chunk->constantsCount] = value;
    return chunk->constantsCount++;
}

//Free chunk memory
void freeChunk(Chunk* chunk)
{
    free(chunk->code);
    free(chunk->constants);
    initChunk(chunk);
}