#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

void printObject( Obj* o ) {
    switch( o->type ) {
        case OBJ_STRING: printString( (ObjString*)o ); return;
        case OBJ_UPVALUE: printUpvalue( (ObjUpvalue*)o ); return;
        case OBJ_FUNCTION: printFunction( (ObjFunction*)o ); return;
        case OBJ_NATIVE: printf( "<native>" ); return;
        case OBJ_CLOSURE: printFunction( ((ObjClosure*)o)->function ); return;
        default: printf( "obj<%p>", o ); return;
    }
}

void printObjectType( ObjType type ) {
    switch( type ) {
        case OBJ_STRING: printf( "OBJ_STRING" ); return;
        case OBJ_UPVALUE: printf( "OBJ_UPVALUE" ); return;
        case OBJ_FUNCTION: printf( "OBJ_FUNCTION" ); return;
        case OBJ_NATIVE: printf( "OBJ_NATIVE" ); return;
        case OBJ_CLOSURE: printf( "OBJ_CLOSURE" ); return;
        default: printf( "OBJ_UNKNOWN" ); return;
    }
}

void printObjectDebug( Obj* o ) {
    printObjectType( o->type );
    printf( " " );
    printObject( o );
    printf( " @ %p", (void*)o );
}

void printString( ObjString* s ) { printf( "\"%.*s\"", (int)s->len, s->buf ); }
void printStringToErr( ObjString* s ) { fprintf( stderr, "\"%.*s\"", (int)s->len, s->buf ); }

void printFunction( ObjFunction* f ) {
    if( f->name ) printf( "%.*s()", (int)f->name->len, f->name->buf ); else printf( "()" );
}

void printUpvalue( ObjUpvalue* upvalue ) {
    printf( "*" );
    if( upvalue->location ) printValue( *upvalue->location ); else printf( "NULL" );
}

static Obj* allocateObject( size_t size, ObjType type ) {
    // allocate memory
    Obj* obj = allocate( size );

    // set type
    obj->type = type;
    obj->isMarked = false;

    // insert into VM's object list
    obj->next = vm.objects;
    vm.objects = obj;

    // log the allocation
    #ifdef DEBUG_LOG_GC
    printf( "allocate " );
    printObjectType( type );
    printf( " @ %p\n", (void*)obj ); // %zu for size
    #endif
    return obj;
}

ObjFunction* newFunction() {
    ObjFunction* function = (ObjFunction*)allocateObject( sizeof( ObjFunction ), OBJ_FUNCTION );
    function->obj.type = OBJ_FUNCTION;
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk( &function->chunk );
    return function;
}

ObjNative* newNative( NativeFn function ) {
    ObjNative* native = (ObjNative*)allocateObject( sizeof( ObjNative ), OBJ_NATIVE );
    native->function = function;
    return native;
}

ObjClosure* newClosure( ObjFunction* function ) {
    // allocate upvalue buffer
    int upvalueCount = function->upvalueCount;
    ObjUpvalue** upvalues = (ObjUpvalue**)allocate( sizeof( ObjUpvalue*) * upvalueCount );
    for( int i = 0; i < upvalueCount; i++ ) upvalues[i] = NULL;

    // allocate closure
    ObjClosure* closure = (ObjClosure*)allocateObject( sizeof( ObjClosure ), OBJ_CLOSURE );
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = upvalueCount;
    return closure;
}

static uint32_t hashStringFNV1a32( const char* key, size_t len, uint32_t hash ) {
    for( size_t i = 0; i < len; i++ ) {
        hash ^= (uint8_t)key[i];
        hash *= HASH_PRIME;
    }
    return hash;
}

ObjString* makeString( const char* s, size_t len ) {
    return concatStrings( s, len, NULL, 0 );
}

ObjString* concatStrings( const char* s1, size_t len1, const char* s2, size_t len2 ) {
    // compute hash
    uint32_t hash = hashStringFNV1a32( s2, len2, hashStringFNV1a32( s1, len1, HASH_SEED ) );

    // find string in table
    ObjString* obj = tableFindString( &vm.strings, hash, s1, len1, s2, len2 );
    if( NULL != obj ) return obj;

    // otherwise, allocate a new string
    int len = len1 + len2;
    obj = (ObjString*)allocateObject( sizeof( ObjString ) + len, OBJ_STRING );
    obj->len = len;
    obj->hash = hash;
    memcpy( obj->buf, s1, len1 );
    memcpy( obj->buf + len1, s2, len2 );

    // intern it
    push( OBJ_VAL( obj ) ); // ensure GC can see this object BEFORE we call tableSet (which might trigger a GC)
    tableSet( &vm.strings, obj, NIL_VAL );
    pop();
    return obj;
}

ObjUpvalue* newUpvalue( Value* slot ) {
    ObjUpvalue* upvalue = (ObjUpvalue*)allocateObject( sizeof( ObjUpvalue ), OBJ_UPVALUE );
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}
