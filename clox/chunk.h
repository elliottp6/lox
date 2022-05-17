#pragma once
#include "common.h"

// types
typedef enum {
    OP_RETURN
} OpCode;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;
} Chunk;

// functions
void initChunk( Chunk* chunk );
void freeChunk( Chunk* chunk );
void writeChunk( Chunk* chunk, uint8_t byte );
