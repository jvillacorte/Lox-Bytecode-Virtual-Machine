#include <stdio.h>
#include "vm.h"
#include "value.h"
#include "compiler.h"
#include "debug.h"

//Reset stack pointer
static void resetStack(VM* vm)
{
    vm->stackTop = vm->stack;
}

//Initialize VM
void initVM(VM* vm)
{
    resetStack(vm);
}

//Cleanup VM
void freeVM(VM* vm)
{
    (void)vm;
}

//Push value
static void push(VM* vm, Value value)
{
    *vm->stackTop = value;
    vm->stackTop++;
}

//Pop value
static Value pop(VM* vm)
{
    vm->stackTop--;
    return *vm->stackTop;
}

//Main execution loop
static InterpretResult run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants[READ_BYTE()])

#define BINARY_OP(op) \
    do \
    { \
        Value b = pop(vm); \
        Value a = pop(vm); \
        push(vm, a op b); \
    } while (0)

    for (;;)
    {
        uint8_t instruction = READ_BYTE();

        switch (instruction)
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT();
                push(vm, constant);
                break;
            }

            case OP_ADD:
                BINARY_OP(+);
                break;

            case OP_SUBTRACT:
                BINARY_OP(-);
                break;

            case OP_MULTIPLY:
                BINARY_OP(*);
                break;

            case OP_DIVIDE:
                BINARY_OP(/);
                break;

            case OP_NEGATE:
                push(vm, -pop(vm));
                break;

            case OP_PRINT:
                printValue(pop(vm));
                printf("\n");
                break;

            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

//Compile and run source
InterpretResult interpret(VM* vm, const char* source, int debugMode)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    if (debugMode)
    {
        disassembleChunk(&chunk, "script");
        freeChunk(&chunk);
        return INTERPRET_OK;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);

    freeChunk(&chunk);
    return result;
}