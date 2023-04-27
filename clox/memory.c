#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

static void freeObject( Obj* o ) {
    // trace object freeing
    printObject( o );
    printf( ", " );
    
    // switch on type so we can track VM memory usage
    switch( o->type ) {        
        case OBJ_STRING: deallocate( o, sizeof( ObjString ) + ((ObjString*)o)->len ); break;
        case OBJ_NATIVE: deallocate( o, sizeof( ObjNative ) ); break;
        case OBJ_CLOSURE: deallocate( o, sizeof( ObjClosure ) ); break;
        case OBJ_FUNCTION: {
            ObjFunction* f = (ObjFunction*)o;
            freeChunk( &f->chunk );
            deallocate( o, sizeof( ObjFunction ) );
            break;
        }
        default: break; // unreachable
    }
}

void freeObjects() {
    printf( "=> free objects: " );
    Obj* obj = vm.objects;
    while( NULL != obj ) {
        Obj* next = obj->next;
        freeObject( obj );
        obj = next;
    }
    vm.objects = NULL;
    printf( "\n" );
}
