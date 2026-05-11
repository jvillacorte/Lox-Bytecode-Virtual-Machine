#include <stdio.h>
#include "value.h"

//Create bool value
Value boolValue(int boolean)
{
    Value value;
    value.type = VAL_BOOL;
    value.as.boolean = boolean;
    return value;
}

//Create nil value
Value nilValue()
{
    Value value;
    value.type = VAL_NIL;
    return value;
}

//Create number value
Value numberValue(double number)
{
    Value value;
    value.type = VAL_NUMBER;
    value.as.number = number;
    return value;
}

//Create string value
Value stringValue(ObjString* string)
{
    Value value;
    value.type = VAL_STRING;
    value.as.string = string;
    return value;
}

//Check truthiness
int isFalsey(Value value)
{
    if (value.type == VAL_NIL)
    {
        return 1;
    }

    if (value.type == VAL_BOOL)
    {
        return !value.as.boolean;
    }

    return 0;
}

//Check equality
int valuesEqual(Value a, Value b)
{
    if (a.type != b.type)
    {
        return 0;
    }

    switch (a.type)
    {
        case VAL_BOOL:
            return a.as.boolean == b.as.boolean;

        case VAL_NIL:
            return 1;

        case VAL_NUMBER:
            return a.as.number == b.as.number;

        case VAL_STRING:
            return a.as.string == b.as.string;
    }

    return 0;
}

//Print value
void printValue(Value value)
{
    switch (value.type)
    {
        case VAL_BOOL:
            printf(value.as.boolean ? "true" : "false");
            break;

        case VAL_NIL:
            printf("nil");
            break;

        case VAL_NUMBER:
            printf("%g", value.as.number);
            break;

        case VAL_STRING:
            printf("%s", value.as.string->chars);
            break;
    }
}