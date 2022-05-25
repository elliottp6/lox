#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

void printObject( Obj* obj ) {
    printf( "obj<%p>", obj );
    switch( obj->type ) {
        case OBJ_STRING: {
            ObjString* str = (ObjString*)obj;
            printf( "\"%.*s\"", (int)str->len, str->buf );
            break;
        }
    }
}

ObjString* makeStringObject( const char* chars, size_t len ) {
    ObjString* obj = allocate( sizeof( ObjString ) + len );
    obj->obj.type = OBJ_STRING;
    obj->len = len;
    memcpy( obj->buf, chars, len );
    return obj;
}
