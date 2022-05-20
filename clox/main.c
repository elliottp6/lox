#include <stdio.h>
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main( __attribute__((unused)) int argc, __attribute__((unused)) const char* argv[] ) {
    // init virtual machine
    initVM();
    
    // init chunk
    Chunk chunk;
    initChunk( &chunk );
    
    // 1.2 + 3.4
    writeChunk( &chunk, OP_CONSTANT, 123 );
    writeChunk( &chunk, (uint8_t)addConstant( &chunk, 1.2 ), 123 );
    writeChunk( &chunk, OP_CONSTANT, 123 );
    writeChunk( &chunk, (uint8_t)addConstant( &chunk, 3.4 ), 123 );
    writeChunk( &chunk, OP_ADD, 123 );
    
    // divide by 5.6
    writeChunk( &chunk, OP_CONSTANT, 123 );
    writeChunk( &chunk, (uint8_t)addConstant( &chunk, 5.6 ), 123 );
    writeChunk( &chunk, OP_DIVIDE, 123 );

    // negate
    writeChunk( &chunk, OP_NEGATE, 123 );
    writeChunk( &chunk, OP_RETURN, 123 );

    // disassemble
    disassembleChunk( &chunk, "disassemble chunk" );

    // interpret
    printf( "== interpret chunk ==\n" );
    interpret( &chunk );

    // done
    freeChunk( &chunk );
    freeVM();
    return 0;
}
