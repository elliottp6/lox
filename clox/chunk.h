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
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
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
    OP_JUMP, // forward branch
    OP_JUMP_IF_FALSE, // forward branch
    OP_LOOP, // backward branch
    OP_CALL, // function call
    OP_CLOSURE, // closure creation
    OP_CLOSE_UPVALUE, // upvalue creation
    OP_RETURN,
    OP_CLASS, // class creation
} OpCode;

typedef struct {
    size_t capacity, count;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

// functions
void initChunk( Chunk* chunk );
void freeChunk( Chunk* chunk );
void writeChunk( Chunk* chunk, uint8_t byte, int line );
size_t addConstant( Chunk* chunk, Value value ); // returns the constant's offset
void printConstants( Chunk* chunk );
