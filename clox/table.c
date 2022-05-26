#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

void initTable( Table* table ) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable( Table* table ) {
    freeArray( sizeof( Entry ), table->entries, table->capacity );
    initTable( table );
}

// find entry via linear probing
// this relies on string interning, since we're using reference equality for the keys (rather than calling stringsEqual)
static Entry* findEntry( Entry* entries, size_t capacity, ObjString* key ) {
    for( size_t i = key->hash % capacity;; i = (i + 1) % capacity) {
        Entry* e = &entries[i];
        if( e->key == key || e->key == NULL ) return e;
    }
}

void tableAddAll( Table* from, Table* to ) {
    for( size_t i = 0; i < from->capacity; i++ ) {
        Entry* entry = &from->entries[i];
        if( NULL != entry->key ) tableSet( to, entry->key, entry->value );
    }
}

static void adjustCapacity( Table* table, size_t capacity ) {
    // allocate a zeroed-out array of entries
    // this works because a zeroed-out Entry has a null key and a NIL value
    Entry* entries = zallocate( sizeof( Entry ) * capacity );

    // insert existing entries into the new table
    for( size_t i = 0; i < table->capacity; i++ ) {
        // get old entry
        Entry* entry = &table->entries[i];
        if( NULL == entry->key ) continue;
        
        // insert into new table
        Entry* dest = findEntry( entries, capacity, entry->key );
        dest->key = entry->key;
        dest->value = entry->value;
    }

    // free old entries buffer
    freeArray( sizeof( Entry ), table->entries, table->capacity );

    // set new entries buffer
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet( Table* table, ObjString* key, Value value ) {
    // grow the table if we exceed our max load
    if( table->count + 1 > table->capacity * TABLE_MAX_LOAD )
        adjustCapacity( table, growCapacity( table->capacity ) );

    // get table entry for this key
    Entry* entry = findEntry( table->entries, table->capacity, key );

    // set the entry
    bool add = NULL == entry->key;
    entry->key = key;
    entry->value = value;

    // return 'true' if we added a new entry
    if( add ) table->count++;
    return add;
}

bool tableGet( Table* table, ObjString* key, Value* value ) {
    // this ensures we don't access the bucket array when it's NULL
    if( 0 == table->count ) return false;

    // otherwise, find the entry
    Entry* entry = findEntry( table->entries, table->capacity, key );
    if( NULL == entry->key ) return false;

    // value found, so set it
    *value = entry->value;
    return true;
}
