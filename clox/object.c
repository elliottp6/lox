#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

void printObject( Obj* obj ) {
    switch( obj->type ) {
        case OBJ_STRING: printString( (ObjString*)obj ); return;
        default: printf( "obj<%p>", obj ); return;
    }
}

void printString( ObjString* str ) { printf( "\"%.*s\"@%p", (int)str->len, str->buf, str ); }

static Obj* allocateObject( size_t size ) {
    // allocate memory
    Obj* obj = allocate( size );

    // insert into VM's object list
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

// 32-bit fnv1a
static uint32_t hashString( const char* key, size_t len, uint32_t hash ) {
    for( size_t i = 0; i < len; i++ ) {
        hash ^= (uint8_t)key[i];
        hash *= HASH_PRIME;
    }
    return hash;
}

ObjString* makeString( const char* s, size_t len ) {
    return concatStrings( s, len, NULL, 0 );
}

ObjString* concatStrings( const char* s1, size_t len1, const char* s2, size_t len2 ) {
    // compute hash
    uint32_t hash = hashString( s2, len2, hashString( s1, len1, HASH_SEED ) );

    // find string in table
    ObjString* obj = tableFindString( &vm.strings, hash, s1, len1, s2, len2 );
    if( NULL != obj ) return obj;

    // otherwise, allocate a new string
    int len = len1 + len2;
    obj = (ObjString*)allocateObject( sizeof( ObjString ) + len );
    obj->obj.type = OBJ_STRING;
    obj->len = len;
    obj->hash = hash;
    memcpy( obj->buf, s1, len1 );
    memcpy( obj->buf + len1, s2, len2 );

    // intern it
    tableSet( &vm.strings, obj, NIL_VAL );
    return obj;
}
