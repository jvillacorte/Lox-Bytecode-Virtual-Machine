#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

//Print full chunk
void disassembleChunk(Chunk* chunk, const char* name);

//Print one instruction
int disassembleInstruction(Chunk* chunk, int offset);

#endif