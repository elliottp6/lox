#include <stdio.h>
#include "memory.h"
#include "value.h"

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
        case VAL_BOOL: printf( AS_BOOL( value ) ? "true" : "false" ); break;
        case VAL_NIL: printf( "nil" ); break;
        case VAL_NUMBER: printf( "%g", AS_NUMBER( value ) ); break;
    }
}
