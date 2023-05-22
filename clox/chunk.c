#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vm.h"

void initChunk( Chunk* chunk ) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray( &chunk->constants );
}

void writeChunk( Chunk* chunk, uint8_t byte, int line ) {
    // resize buffer
    if( chunk->capacity < chunk->count + 1 ) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = growCapacity( chunk->capacity );
        chunk->code = growArray( sizeof( uint8_t ), chunk->code, oldCapacity, chunk->capacity );
        chunk->lines = growArray( sizeof( int ), chunk->lines, oldCapacity, chunk->capacity );
    }

    // insert byte & line
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;

    // increment count
    chunk->count++;
}

void freeChunk( Chunk* chunk ) {
    freeArray( sizeof( uint8_t ), chunk->code, chunk->capacity );
    freeArray( sizeof( int ), chunk->lines, chunk->capacity );
    freeValueArray( &chunk->constants );
    initChunk( chunk );
}

size_t addConstant( Chunk* chunk, Value value ) {
    push( value ); // ensure GC can see this value BEFORE we call writeValueArray (which may trigger a GC)
    writeValueArray( &chunk->constants, value );
    pop( value );
    return chunk->constants.count - 1;
}

void printConstants( Chunk* chunk ) {
    printf( "== chunk constants ==\n" );
    size_t count = chunk->constants.count;
    Value* values = chunk->constants.values;
    for( size_t i = 0; i < count; i++ ) {
        printf( "%zu: ", i );
        printValue( values[i] );
        printf( "\n" );
    }
}
