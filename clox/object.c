#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static bool stringsEqual( ObjString* a, ObjString* b ) {
    return a->len == b->len && 0 == memcmp( a->buf, b->buf, a->len );
}

bool objectsEqual( Obj* a, Obj* b ) {
    if( a->type != b->type ) return false;
    switch( a->type ) {
        case OBJ_STRING: return stringsEqual( (ObjString*)a, (ObjString*)b );
        default: return a == b;
    }
}

void printObject( Obj* obj ) {
    switch( obj->type ) {
        case OBJ_STRING: {
            ObjString* str = (ObjString*)obj;
            printf( "\"%.*s\"", (int)str->len, str->buf );
            return;
        }
        default:
            printf( "obj<%p>", obj );
            return;
    }
}

static Obj* allocateObject( size_t size ) {
    // allocate memory
    Obj* obj = allocate( size );

    // add to VM's global list (so we can release on call to freeVM)
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

ObjString* makeString( const char* chars, size_t len ) {
    ObjString* obj = (ObjString*)allocateObject( sizeof( ObjString ) + len );
    obj->obj.type = OBJ_STRING;
    obj->len = len;
    memcpy( obj->buf, chars, len );
    return obj;
}

ObjString* concatStrings( ObjString* a, ObjString* b ) {
    size_t len = a->len + b->len;
    ObjString* obj = allocate( sizeof( ObjString ) + len );
    obj->obj.type = OBJ_STRING;
    obj->len = len;
    memcpy( obj->buf, a->buf, a->len );
    memcpy( obj->buf + a->len, b->buf, b->len );
    return obj;
}
