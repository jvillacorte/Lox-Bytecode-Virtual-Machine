#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "vm.h"

//Compile source into bytecode chunk
int compile(const char* source, Chunk* chunk, VM* vm);

#endif