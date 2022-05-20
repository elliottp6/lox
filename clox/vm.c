#include <stdio.h>
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

void initVM() {

}

void freeVM() {

}

static InterpretResult run() {
    // macros
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    // main loop
    for( uint8_t instruction;; ) {
        // print instruction (if trading is enabled)
        #ifdef DEBUG_TRACE_EXECUTION
            disassembleInstruction( vm.chunk, (size_t)(vm.ip - vm.chunk->code) );
        #endif

        // interpret instruction
        switch( instruction = READ_BYTE() ) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue( constant );
                printf( "\n" );
                break;
            }
            case OP_RETURN: {
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
