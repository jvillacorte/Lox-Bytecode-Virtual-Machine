#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "object.h"

//Value types
typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING
} ValueType;

//Runtime value
typedef struct
{
    ValueType type;

    union
    {
        int boolean;
        double number;
        ObjString* string;
    } as;

} Value;

//Create bool value
Value boolValue(int boolean);

//Create nil value
Value nilValue();

//Create number value
Value numberValue(double number);

//Create string value
Value stringValue(ObjString* string);

//Check truthiness
int isFalsey(Value value);

//Check equality
int valuesEqual(Value a, Value b);

//Print value
void printValue(Value value);

#endif