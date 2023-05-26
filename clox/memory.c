#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "compiler.h"
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

static void markArray( ValueArray* array ) {
    for( size_t i = 0; i < array->count; i++ ) {
        markValue( array->values[i] );
    }
}

static void blackenObject( Obj* object ) {
    #ifdef DEBUG_LOG_GC
    printf( "blacken " );
    printObjectDebug( object );
    printf( "\n" );
    #endif

    switch( object->type ) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject( (Obj*)closure->function );
            for( int i = 0; i < closure->upvalueCount; i++ ) markObject( (Obj*)closure->upvalues[i] );
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject( (Obj*)function->name );
            markArray( &function->chunk.constants );
            break;
        }
        case OBJ_UPVALUE:
            markValue( ((ObjUpvalue*)object)->closed );
            break;
        case OBJ_NATIVE: case OBJ_STRING: break;
    }
}

void markObject( Obj* object ) {
    // ignore null objects, and objects that we've already marked as gray
    if( NULL == object ) return;
    if( object->isMarked ) {
        #ifdef DEBUG_LOG_GC
        printf( "remark " );
        printObjectDebug( object );
        printf( "\n" );
        #endif
        return;
    }

    #ifdef DEBUG_LOG_GC
    printf( "mark " );
    printObjectDebug( object );
    printf( "\n" );
    #endif

    // mark the object
    object->isMarked = true;

    // grow capacity of the grayStack as needed
    if( vm.grayCapacity < vm.grayCount + 1 ) {
        vm.grayCapacity = growCapacity( vm.grayCapacity );
        vm.grayStack = (Obj**)realloc( vm.grayStack, sizeof(Obj*) * vm.grayCapacity );
        if( NULL == vm.grayStack ) exit( 1 );
    }

    // add the object to the grayStack
    vm.grayStack[vm.grayCount++] = object;
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
    printf( "free " );
    printObjectDebug( o );
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
    #ifdef DEBUG_LOG_GC
    printf( "=> free objects:\n" );
    #endif
    Obj* obj = vm.objects;
    while( NULL != obj ) {
        Obj* next = obj->next;
        freeObject( obj );
        obj = next;
    }
    vm.objects = NULL;

    // free the VM's grayStack
    free( vm.grayStack );
}

static void markRoots() {
    // mark every object reference to by the stack
    for( Value* slot = vm.stack; slot < vm.stackTop; slot++ ) {
        markValue( *slot );
    }

    // mark the closures inside of each CallFrame
    for( int i = 0; i < vm.frameCount; i++ ) {
        markObject( (Obj*)vm.frames[i].closure );
    }

    // mark the open upvalue list
    for( ObjUpvalue* upvalue = vm.openUpvalues; NULL != upvalue; upvalue = upvalue->next ) {
        markObject( (Obj*)upvalue );
    }

    // mark the table of global variables
    markTable( &vm.globals );

    // mark the roots of the compiler
    markCompilerRoots();
}

// remove and blacken each object from the grayStack, one-at-a-time
// note that blackening an object may require adding more, new objects to the grayStack
static void traceReferences() {
    while( vm.grayCount > 0 ) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject( object );
    }
}

static void sweep() {
    // check each object
    for( Obj *obj = vm.objects, *previous = NULL; NULL != obj; ) {
        if( obj->isMarked ) {
            // skip object
            #ifdef DEBUG_LOG_GC
            printf( "unmark: " );
            printObjectDebug( (void*)obj );
            printf( "\n" );
            #endif
            obj->isMarked = false; // set unmarked (for next mark phase)
            previous = obj;
            obj = obj->next;
        } else {
            // remove objects from vm.objects linked list
            Obj *unreached = obj;
            obj = obj->next;
            if( NULL != previous ) previous->next = obj; else vm.objects = obj;

            // free the unreachable object
            freeObject( unreached );
        }
    }
}

void collectGarbage() {
    #ifdef DEBUG_LOG_GC
    printf( "-- gc begin\n" );
    #endif

    // mark phase of mark-end-sweep
    markRoots();
    traceReferences();
    tableRemoveWhite( &vm.strings );
    sweep();

    #ifdef DEBUG_LOG_GC
    printf( "-- gc end\n" );
    #endif
}
