#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

#define STACK_MAX 256

//Virtual Machine structure
typedef struct
{
    Chunk* chunk;
    uint8_t* ip;

    Value stack[STACK_MAX];
    Value* stackTop;

} VM;

//Execution result
typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

//Initialize VM
void initVM(VM* vm);

//Free VM
void freeVM(VM* vm);

//Run source code
InterpretResult interpret(VM* vm, const char* source, int debugMode);

#endif