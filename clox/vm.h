#pragma once
#include "chunk.h"
#include "table.h"
#include "value.h"
#include "object.h"

#define FRAMES_MAX 64 // maximum call depth
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // up to 256 variables for each function call (probably overkill?)

typedef struct {
    ObjFunction* function; // current function being called
    uint8_t* ip; // the instruction pointer for the caller (i.e. where to return to after we finish function execution)
    Value* slots; // points into the VM's Value stack @ the point where the function's arguments begin
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX]; // one frame for every function call
    int frameCount; // the call depth
    Chunk* chunk; // current chunk that we're executing
    uint8_t* ip; // pointer to the current instruction in the chunk
    Value stack[STACK_MAX]; // our value stack
    Value* stackTop; // pointer to the latest value in the stack
    Table globals, strings; // for global variables, for string interning
    Obj* objects; // for keeping track of all objects, so we can GC them
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
