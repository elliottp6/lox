#pragma once
#include "common.h"
#include "value.h"

// types
typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
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
