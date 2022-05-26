#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "vm.h"

static void* reallocate( void* buffer, size_t oldSize, size_t newSize ) {
    // no bytes requested: free memory
    if( 0 == newSize ) {
        free( buffer );
        return NULL;
    }

    // otherwise: allocate/resize memory
    void* newBuffer = realloc( buffer, newSize );
    if( NULL == newBuffer ) exit( 1 ); // out-of-memory!
    return newBuffer;
}

void* allocate( size_t size ) {
    return reallocate( NULL, 0, size );
}

void* zallocate( size_t size ) {
    void* buffer = allocate( size );
    memset( buffer, 0, size );
    return buffer;
}

void deallocate( void* buffer, size_t size ) {
    reallocate( buffer, size, 0 );
}

size_t growCapacity( size_t capacity ) {
    return capacity < 8 ? 8 : capacity << 1;
}

void* growArray( size_t typeSize, void* array, size_t oldCapacity, size_t newCapacity ) {
    size_t oldSize = typeSize * oldCapacity, newSize = typeSize * newCapacity;
    return reallocate( array, oldSize, newSize );
}

void freeArray( size_t typeSize, void* array, size_t capacity ) {
    size_t size = typeSize * capacity;
    deallocate( array, size );
}

static void freeObject( Obj* obj ) {
    switch( obj->type ) {
        case OBJ_STRING: {
            ObjString* str = (ObjString*)obj;
            deallocate( str, sizeof( ObjString ) + str->len );
            break;
        }
        default:
            break; // unreachable
    }
}

void freeObjects() {
    Obj* obj = vm.objects;
    while( NULL != obj ) {
        Obj* next = obj->next;
        freeObject( obj );
        obj = next;
    }
    vm.objects = NULL;
}
