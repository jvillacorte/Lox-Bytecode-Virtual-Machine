#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256
#define GLOBAL_MAX 256

//Global variable entry
typedef struct
{
    ObjString* name;
    Value value;
} Global;

//Virtual Machine structure
typedef struct
{
    Chunk* chunk;
    uint8_t* ip;

    Value stack[STACK_MAX];
    Value* stackTop;

    Global globals[GLOBAL_MAX];
    int globalCount;

    Table strings;

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

//Intern string
ObjString* internString(VM* vm, const char* chars, int length);

//Run source code
InterpretResult interpret(VM* vm, const char* source, const char* name, int debugMode);

#endif