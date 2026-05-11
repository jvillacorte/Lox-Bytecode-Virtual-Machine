#include <stdlib.h>
#include "object.h"

//Hash string contents
uint32_t hashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

//Create string object
ObjString* allocateString(char* chars, int length, uint32_t hash)
{
    ObjString* string = malloc(sizeof(ObjString));
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    return string;
}

//Free string object
void freeString(ObjString* string)
{
    free(string->chars);
    free(string);
}