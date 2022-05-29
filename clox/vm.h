#pragma once
#include "chunk.h"
#include "table.h"
#include "value.h"
#include "object.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals; // for global variables
    Table strings; // for string interning
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm; // vm variable exposed to entire app

void initVM();
void freeVM();
InterpretResult interpret_chunk( Chunk* chunk );
InterpretResult interpret_source( const char* source );
void push( Value value );
Value pop();
