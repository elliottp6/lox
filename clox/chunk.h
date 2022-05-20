#pragma once
#include "common.h"
#include "value.h"

// types
typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    size_t capacity;
    size_t count;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

// functions
void initChunk( Chunk* chunk );
void freeChunk( Chunk* chunk );
void writeChunk( Chunk* chunk, uint8_t byte, int line );
size_t addConstant( Chunk* chunk, Value value ); // returns the constant's offset
