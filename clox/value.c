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
    #ifdef NAN_BOXING
    if( IS_BOOL( value ) ) {
        printf( AS_BOOL( value ) ? "true" : "false" );
    } else if( IS_NIL( value ) ) {
        printf( "nil" );
    } else if( IS_NUMBER( value ) ) {
        printf( "%g", AS_NUMBER( value ) );
    } else if( IS_OBJ( value ) ) {
        printObject( AS_OBJ( value ) );
    } else if( IS_ERROR( value ) ) {
        switch( AS_ERROR( value ) ) {
            case COMPILE_ERROR: printf( "COMPILE ERROR" ); return;
            case RUNTIME_ERROR: printf( "Runtime Error" ); return;
            default: printf( "Unknown Error %d", AS_ERROR( value ) ); return;
        }
    }
    #else
    switch( value.type ) {
        case VAL_NIL: printf( "nil" ); return;
        case VAL_BOOL: printf( AS_BOOL( value ) ? "true" : "false" ); return;
        case VAL_NUMBER: printf( "%g", AS_NUMBER( value ) ); return;
        case VAL_OBJ: printObject( AS_OBJ( value ) ); return;
        case VAL_ERROR: {
            switch( AS_ERROR( value ) ) {
                case COMPILE_ERROR: printf( "COMPILE ERROR" ); return;
                case RUNTIME_ERROR: printf( "Runtime Error" ); return;
                default: printf( "Unknown Error %d", AS_ERROR( value ) ); return;
            }
        }
    }
    #endif
}

bool valuesEqual( Value a, Value b ) {
    #ifdef NAN_BOXING
    if( IS_NUMBER( a ) && IS_NUMBER( b ) ) { // must use this here to ensure that NaN does not equal itself (or, skip it?)
        return AS_NUMBER( a ) == AS_NUMBER( b );
    }
    return a == b;
    #else
    if( a.type != b.type ) return false;
    switch( a.type ) {
        case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:       return true;
        case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b); // reference equality (also, works for string equality due to interning)
        case VAL_ERROR:     return AS_ERROR(a) == AS_ERROR(b);
    }
    return false; // unreachable
    #endif
}
