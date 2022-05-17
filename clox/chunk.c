#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void initChunk( Chunk* chunk ) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
}

void writeChunk( Chunk* chunk, uint8_t byte ) {
    // resize the buffer if needed
    if( chunk->capacity < chunk->count + 1 ) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = growCapacity( chunk->capacity );
        chunk->code = growArray( sizeof( uint8_t ), chunk->code, oldCapacity, chunk->capacity );
    }

    // insert the byte & increment count
    chunk->code[chunk->count++] = byte;
}

void freeChunk( Chunk* chunk ) {
    freeArray( sizeof( uint8_t ), chunk->code, chunk->capacity );
    initChunk( chunk );
}
