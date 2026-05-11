#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdint.h>

//Interned string object
typedef struct
{
    int length;
    char* chars;
    uint32_t hash;
} ObjString;

//Hash string contents
uint32_t hashString(const char* key, int length);

//Create string object
ObjString* allocateString(char* chars, int length, uint32_t hash);

//Free string object
void freeString(ObjString* string);

#endif