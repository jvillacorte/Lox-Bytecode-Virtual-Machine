#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "object.h"

//Hash table entry
typedef struct
{
    ObjString* key;
} Entry;

//Intern table
typedef struct
{
    int count;
    int capacity;
    Entry* entries;
} Table;

//Initialize table
void initTable(Table* table);

//Free table
void freeTable(Table* table);

//Find existing string
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

//Insert string
void tableSet(Table* table, ObjString* key);

#endif