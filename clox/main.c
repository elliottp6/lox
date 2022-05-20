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
    
    // write data & code into chunk
    size_t constantIndex = addConstant( &chunk, 1.2 );
    writeChunk( &chunk, OP_CONSTANT, 123 );
    writeChunk( &chunk, (uint8_t)constantIndex, 123 );
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
