#include <stdio.h>
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"

VM vm; // global variable!

static void resetStack() {
    vm.stackTop = vm.stack;
}

void push( Value value ) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}


void initVM() {
    resetStack();
}

void freeVM() {
}

static InterpretResult run() {
    // if we're tracing, show it
    #ifdef DEBUG_TRACE_EXECUTION
    printf( "== execution trace ==\n" );
    #endif

    // macros
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    // this macro looks strange, but it's a way to define a block that permits a semicolon at the end
    #define BINARY_OP(op) \
        do { \
            double b = pop(); \
            double a = pop(); \
            push( a op b ); \
        } while( false )

    // main loop
    for( uint8_t instruction;; ) {
        // trace execution
        #ifdef DEBUG_TRACE_EXECUTION
            // print instruction info
            disassembleInstruction( vm.chunk, (size_t)(vm.ip - vm.chunk->code) );

            // print stack contents
            if( vm.stack < vm.stackTop ) {
                printf( " [" );
                for( Value* slot = vm.stack; slot < vm.stackTop; slot++ )  {
                    if( slot != vm.stack ) printf( ", " );
                    printValue( *slot );
                }
                printf( "]" );
            }

            // newline
            printf( "\n" );
        #endif

        // interpret instruction
        switch( instruction = READ_BYTE() ) {
            case OP_CONSTANT: push( READ_CONSTANT() ); break;
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NEGATE: push( -pop() ); break;
            case OP_RETURN: printValue( pop() ); printf( "\n" ); return INTERPRET_OK;
        }
    }

    // undefine macros
    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
}

InterpretResult interpret_chunk( Chunk* chunk ) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

InterpretResult interpret_source( const char* source ) {
    // create chunk
    Chunk chunk;
    initChunk( &chunk );

    // compile source into chunk
    if( !compile( source, &chunk ) ) {
        freeChunk( &chunk );
        return INTERPRET_COMPILE_ERROR;
    }

    // put chunk into VM
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    // run VM
    InterpretResult result = run();

    // done
    freeChunk( &chunk );
    return result;
}
