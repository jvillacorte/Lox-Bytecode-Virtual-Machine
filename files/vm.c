#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    vm->globalCount = 0;
    initTable(&vm->strings);
}

//Cleanup VM
void freeVM(VM* vm)
{
    freeTable(&vm->strings);
}

//Intern string
ObjString* internString(VM* vm, const char* chars, int length)
{
    uint32_t hash = hashString(chars, length);

    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);

    if (interned != NULL)
    {
        return interned;
    }

    char* heapChars = malloc(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    ObjString* string = allocateString(heapChars, length, hash);
    tableSet(&vm->strings, string);

    return string;
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

//Peek stack value
static Value peek(VM* vm, int distance)
{
    return vm->stackTop[-1 - distance];
}

//Find global variable
static int findGlobal(VM* vm, ObjString* name)
{
    for (int i = 0; i < vm->globalCount; i++)
    {
        if (vm->globals[i].name == name)
        {
            return i;
        }
    }

    return -1;
}

//Define global variable
static void defineGlobal(VM* vm, ObjString* name, Value value)
{
    int index = findGlobal(vm, name);

    if (index != -1)
    {
        vm->globals[index].value = value;
        return;
    }

    if (vm->globalCount >= GLOBAL_MAX)
    {
        fprintf(stderr, "Too many global variables.\n");
        return;
    }

    vm->globals[vm->globalCount].name = name;
    vm->globals[vm->globalCount].value = value;
    vm->globalCount++;
}

//Runtime error
static InterpretResult runtimeError(const char* message)
{
    fprintf(stderr, "%s\n", message);
    return INTERPRET_RUNTIME_ERROR;
}

//Check number operands
static int checkNumbers(Value a, Value b)
{
    return a.type == VAL_NUMBER && b.type == VAL_NUMBER;
}

//Main execution loop
static InterpretResult run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_SHORT() \
    (vm->ip += 2, (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]))
#define READ_CONSTANT() (vm->chunk->constants[READ_BYTE()])

#define BINARY_NUMBER_OP(op) \
    do \
    { \
        Value b = pop(vm); \
        Value a = pop(vm); \
        if (!checkNumbers(a, b)) \
        { \
            return runtimeError("Operands must be numbers."); \
        } \
        push(vm, numberValue(a.as.number op b.as.number)); \
    } while (0)

#define BINARY_COMPARE_OP(op) \
    do \
    { \
        Value b = pop(vm); \
        Value a = pop(vm); \
        if (!checkNumbers(a, b)) \
        { \
            return runtimeError("Operands must be numbers."); \
        } \
        push(vm, boolValue(a.as.number op b.as.number)); \
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

            case OP_NIL:
                push(vm, nilValue());
                break;

            case OP_TRUE:
                push(vm, boolValue(1));
                break;

            case OP_FALSE:
                push(vm, boolValue(0));
                break;

            case OP_EQUAL:
            {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, boolValue(valuesEqual(a, b)));
                break;
            }

            case OP_GREATER:
                BINARY_COMPARE_OP(>);
                break;

            case OP_LESS:
                BINARY_COMPARE_OP(<);
                break;

            case OP_ADD:
                BINARY_NUMBER_OP(+);
                break;

            case OP_SUBTRACT:
                BINARY_NUMBER_OP(-);
                break;

            case OP_MULTIPLY:
                BINARY_NUMBER_OP(*);
                break;

            case OP_DIVIDE:
                BINARY_NUMBER_OP(/);
                break;

            case OP_NOT:
                push(vm, boolValue(isFalsey(pop(vm))));
                break;

            case OP_NEGATE:
            {
                Value value = pop(vm);

                if (value.type != VAL_NUMBER)
                {
                    return runtimeError("Operand must be a number.");
                }

                push(vm, numberValue(-value.as.number));
                break;
            }

            case OP_AND:
            {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, boolValue(!isFalsey(a) && !isFalsey(b)));
                break;
            }

            case OP_OR:
            {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, boolValue(!isFalsey(a) || !isFalsey(b)));
                break;
            }

            case OP_PRINT:
                printValue(pop(vm));
                printf("\n");
                break;

            case OP_POP:
                pop(vm);
                break;

            case OP_DEFINE_GLOBAL:
            {
                Value name = READ_CONSTANT();
                Value value = pop(vm);

                defineGlobal(vm, name.as.string, value);
                break;
            }

            case OP_GET_GLOBAL:
            {
                Value name = READ_CONSTANT();
                int index = findGlobal(vm, name.as.string);

                if (index == -1)
                {
                    fprintf(stderr, "Undefined variable '%s'.\n", name.as.string->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(vm, vm->globals[index].value);
                break;
            }

            case OP_SET_GLOBAL:
            {
                Value name = READ_CONSTANT();
                int index = findGlobal(vm, name.as.string);

                if (index == -1)
                {
                    fprintf(stderr, "Undefined variable '%s'.\n", name.as.string->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = peek(vm, 0);
                vm->globals[index].value = value;
                break;
            }


            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                push(vm, vm->stack[slot]);
                break;
            }

            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                vm->stack[slot] = peek(vm, 0);
                break;
            }

            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                vm->ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();

                if (isFalsey(peek(vm, 0)))
                {
                    vm->ip += offset;
                }

                break;
            }

            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                vm->ip -= offset;
                break;
            }

            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_NUMBER_OP
#undef BINARY_COMPARE_OP
}

//Compile and run source
InterpretResult interpret(VM* vm, const char* source, const char* name, int debugMode)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk, vm))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    if (debugMode)
    {
        disassembleChunk(&chunk, name);
        freeChunk(&chunk);
        return INTERPRET_OK;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);

    freeChunk(&chunk);
    return result;
}