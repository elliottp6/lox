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
            printf( "%.*s", (int)str->len, str->buf );
            return;
        }
        default:
            printf( "obj<%p>", obj );
            return;
    }
}

ObjString* makeStringObject( const char* chars, size_t len ) {
    ObjString* obj = allocate( sizeof( ObjString ) + len );
    obj->obj.type = OBJ_STRING;
    obj->len = len;
    memcpy( obj->buf, chars, len );
    return obj;
}
