#include <stdio.h>
#include "common.h"
#include "debug.h"
#include "vm.h"

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
    // macros
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    // main loop
    for( uint8_t instruction;; ) {
        // trace execution
        #ifdef DEBUG_TRACE_EXECUTION
            // print stack contents
            printf( "          " );
            for( Value* slot = vm.stack; slot < vm.stackTop; slot++ )  {
                printf("[ ");   
                printValue( *slot );
                printf(" ]");
            }
            printf( "\n" );
            
            // print instruction info
            disassembleInstruction( vm.chunk, (size_t)(vm.ip - vm.chunk->code) );
        #endif

        // interpret instruction
        switch( instruction = READ_BYTE() ) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push( constant );
                break;
            }
            case OP_RETURN: {
                printValue( pop() );
                printf( "\n" );
                return INTERPRET_OK;
            }
        }
    }

    // undefine macros
    #undef READ_BYTE
    #undef READ_CONSTANT
}

InterpretResult interpret( Chunk* chunk ) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}
