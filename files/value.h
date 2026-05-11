#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

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
        char* string;
    } as;

} Value;

//Create bool value
Value boolValue(int boolean);

//Create nil value
Value nilValue();

//Create number value
Value numberValue(double number);

//Create string value
Value stringValue(const char* chars, int length);

//Check truthiness
int isFalsey(Value value);

//Check equality
int valuesEqual(Value a, Value b);

//Print value
void printValue(Value value);

//Free value if needed
void freeValue(Value value);

#endif