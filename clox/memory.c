#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memory.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#endif

static void* reallocate( void* buffer, size_t oldSize, size_t newSize ) {
    // when we request more memory: run the GC
    if( newSize > oldSize ) {
    #ifdef DEBUG_STRESS_GC
        collectGarbage();
    #endif
    }

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

void markValue( Value value )  {
    if( IS_OBJ( value ) ) markObject( AS_OBJ( value ) );
}

void markObject( Obj* object ) {
    if( NULL == object ) return;

    #ifdef DEBUG_LOG_GC
    printf( "%p mark ", (void*)object );
    printValue( OBJ_VAL( object ) );
    printf( "\n" );
    #endif

    object->isMarked = true;
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
    #ifdef DEBUG_LOG_GC
    printf( "%p free type %d with value ", (void*)o, o->type );
    printObject( o );
    printf( "\n" );
    #endif
    
    // switch on type so we can track VM memory usage
    switch( o->type ) {        
        case OBJ_STRING: deallocate( o, sizeof( ObjString ) + ((ObjString*)o)->len ); break;
        case OBJ_UPVALUE: deallocate( o, sizeof( ObjUpvalue ) ); break;
        case OBJ_NATIVE: deallocate( o, sizeof( ObjNative ) ); break;
        case OBJ_CLOSURE: {
            ObjClosure* c = (ObjClosure*)o;
            freeArray( sizeof( ObjUpvalue* ), c->upvalues, c->upvalueCount );
            deallocate( o, sizeof( ObjClosure ) );
            break;
        }
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

static void markRoots() {
    // mark every object reference to by the stack
    for( Value* slot = vm.stack; slot < vm.stackTop; slot++ ) {
        markValue( *slot );
    }

    // mark the table of global variables
    markTable( &vm.globals );
}

void collectGarbage() {
    #ifdef DEBUG_LOG_GC
    printf( "-- gc begin\n" );
    #endif

    // mark phase of mark-end-sweep
    markRoots();

    #ifdef DEBUG_LOG_GC
    printf( "-- gc end\n" );
    #endif
}
