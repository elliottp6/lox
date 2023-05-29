#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

void initTable( Table* table ) {
    table->load = 0;
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
    // 1st tombstone found
    Entry* t = NULL;

    // linear probe
    for( size_t i = key->hash & (capacity - 1);; i = (i + 1) & (capacity - 1) ) {
        // get entry
        Entry* e = &entries[i];

        // check for available slot
        if( NULL == e->key ) {
            // if not tombstone: we know the value doesn't exist, so return first available slot
            if( IS_NIL( e->value ) ) return NULL != t ? t : e;

            // otherwise: save the tombstone
            else if( NULL == t ) t = e;

        // otherwise: check for a match
        // note that we can use reference equality here b/c all ObjString's are interned
        } else if( e->key == key ) return e;
    }
}

void tableAddAll( Table* from, Table* to ) {
    for( size_t i = 0; i < from->capacity; i++ ) {
        Entry* entry = &from->entries[i];
        if( NULL != entry->key ) tableSet( to, entry->key, entry->value );
    }
}

static void adjustCapacity( Table* table, size_t newCapacity ) {
    // allocate new entries
    // bugfix: old was just: Entry* newEntries = zallocate( sizeof( Entry ) * newCapacity );
    //         but, now we've redefined NIL_VAL b/c of NaN boxing, so we have to set it (cannot rely on NIL being zero anymore)
    Entry* newEntries = allocate( sizeof( Entry ) * newCapacity );
    for( size_t i = 0; i < newCapacity; i++ ) {
        newEntries[i].key = NULL;
        newEntries[i].value = NIL_VAL;
    }

    // insert existing entries into the new table
    size_t newLoad = 0;
    for( size_t i = 0; i < table->capacity; i++ ) {
        // get old entry
        Entry* entry = &table->entries[i];
        if( NULL == entry->key ) continue;
        
        // insert into new table
        Entry* newEntry = findEntry( newEntries, newCapacity, entry->key );
        newEntry->key = entry->key;
        newEntry->value = entry->value;
        newLoad++;
    }

    // free old entries
    freeArray( sizeof( Entry ), table->entries, table->capacity );

    // set new entries buffer
    table->load = newLoad;
    table->capacity = newCapacity;
    table->entries = newEntries;
}

bool tableSet( Table* table, ObjString* key, Value value ) {
    // grow the table if we exceed our max load
    if( table->load + 1 > table->capacity * TABLE_MAX_LOAD )
        adjustCapacity( table, growCapacity( table->capacity ) );

    // get table entry for this key
    Entry* entry = findEntry( table->entries, table->capacity, key );
   
    // increment the table's load (but only if we didn't just replace a tombstone)
    bool isNewKey = NULL == entry->key;
    if( isNewKey && IS_NIL( entry->value ) ) table->load++;

    // set the entry
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableGet( Table* table, ObjString* key, Value* value ) {
    // this ensures we don't access the bucket array when it's NULL
    if( 0 == table->load ) return false;

    // otherwise, find the entry
    Entry* entry = findEntry( table->entries, table->capacity, key );
    if( NULL == entry->key ) return false;

    // value found, so set it
    *value = entry->value;
    return true;
}

// TODO: this should be 'remove', and probably should return the value removed to the caller so it can deal with deallocation (if needed)
bool tableDelete( Table* table, ObjString* key ) {
    // this ensures we don't access the bucket array when it's NULL
    if( 0 == table->load ) return false;

    // otherwise, find the entry
    Entry* entry = findEntry( table->entries, table->capacity, key );
    if( NULL == entry->key ) return false;

    // place a tombstone in the entry
    entry->key = NULL;
    entry->value = BOOL_VAL( true );
    return true;
}

ObjString* tableFindString( Table* table, uint32_t hash, const char* s1, size_t len1, const char* s2, size_t len2 ) {
    // combined length
    size_t len = len1 + len2;
    
    // avoid null pointer access
    if( 0 == table->load ) return NULL;

    // linear probing
    for( uint32_t i = hash & (table->capacity - 1);; i = (i + 1) & (table->capacity - 1) ) {
        // get entry
        Entry* entry = &table->entries[i];
               
        // on null => return if true null
        if( NULL == entry->key ) { if( IS_NIL( entry->value ) ) return NULL; }

        // otherwise: check for a match
        else if( entry->key->hash == hash &&
                 entry->key->len == len &&
                 0 == memcmp( entry->key->buf, s1, len1 ) &&
                 0 == memcmp( entry->key->buf + len1, s2, len2 ) )
            return entry->key;
    }
}

void tableRemoveWhite(Table* table) {
    for( size_t i = 0; i < table->capacity; i++ ) {
        Entry* entry = &table->entries[i];
        if( NULL != entry->key && !entry->key->obj.isMarked ) tableDelete( table, entry->key );
    }
}

void markTable( Table* table ) {
    for( size_t i = 0; i < table->capacity; i++ ) {
        Entry* entry = &table->entries[i];
        markObject( (Obj*)entry->key );
        markValue( entry->value );
    }
}
