#include <stdio.h>
#include "memory.h"
#include "value.h"
#include "object.h"

void initValueArray( ValueArray* array ) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void writeValueArray( ValueArray* array, Value value ) {
    // resize the buffer if needed
    if( array->capacity < array->count + 1 ) {
        size_t oldCapacity = array->capacity;
        array->capacity = growCapacity( array->capacity );
        array->values = growArray( sizeof( Value ), array->values, oldCapacity, array->capacity );
    }

    // insert the byte & increment count
    array->values[array->count++] = value;
}

void freeValueArray( ValueArray* array ) {
    freeArray( sizeof( Value ), array->values, array->capacity );
    initValueArray( array );
}

void printValue( Value value ) {
    switch( value.type ) {
        case VAL_BOOL: printf( AS_BOOL( value ) ? "true" : "false" ); return;
        case VAL_NIL: printf( "nil" ); return;
        case VAL_NUMBER: printf( "%g", AS_NUMBER( value ) ); return;
        case VAL_OBJ: printObject( AS_OBJ( value ) ); return;
    }
}

bool valuesEqual( Value a, Value b ) {
    if( a.type != b.type ) return false;
    switch( a.type ) {
        case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:       return true;
        case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            // TODO: replace this with string comparison
            return AS_OBJ(a) == AS_OBJ(b);
        }
    }
    return false; // unreachable
}
