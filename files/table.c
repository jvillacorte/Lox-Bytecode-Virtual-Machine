#include <stdlib.h>
#include <string.h>
#include "table.h"

#define TABLE_MAX_LOAD 0.75

//Find entry slot
static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
    uint32_t index = key->hash % capacity;

    for (;;)
    {
        Entry* entry = &entries[index];

        if (entry->key == NULL || entry->key == key)
        {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

//Grow capacity
static int growCapacity(int capacity)
{
    return capacity < 8 ? 8 : capacity * 2;
}

//Adjust table capacity
static void adjustCapacity(Table* table, int capacity)
{
    Entry* entries = malloc(sizeof(Entry) * capacity);

    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
    }

    table->count = 0;

    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];

        if (entry->key == NULL)
        {
            continue;
        }

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        table->count++;
    }

    free(table->entries);

    table->entries = entries;
    table->capacity = capacity;
}

//Initialize table
void initTable(Table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

//Free table
void freeTable(Table* table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        if (table->entries[i].key != NULL)
        {
            freeString(table->entries[i].key);
        }
    }

    free(table->entries);
    initTable(table);
}

//Find existing string
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
    if (table->capacity == 0)
    {
        return NULL;
    }

    uint32_t index = hash % table->capacity;

    for (;;)
    {
        Entry* entry = &table->entries[index];

        if (entry->key == NULL)
        {
            return NULL;
        }

        if (entry->key->length == length &&
            entry->key->hash == hash &&
            memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}

//Insert string
void tableSet(Table* table, ObjString* key)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = growCapacity(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry->key == NULL)
    {
        table->count++;
    }

    entry->key = key;
}