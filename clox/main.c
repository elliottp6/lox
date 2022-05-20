#include <stdio.h>
#include "chunk.h"
#include "debug.h"

int main( __attribute__((unused)) int argc, __attribute__((unused)) const char* argv[] ) {
    // init chunk
    Chunk chunk;
    initChunk( &chunk );
    
    // write data & code into chunk
    size_t constantIndex = addConstant( &chunk, 1.2 );
    writeChunk( &chunk, OP_CONSTANT, 123 );
    writeChunk( &chunk, (uint8_t)constantIndex, 123 );
    writeChunk( &chunk, OP_RETURN, 123 );

    // disassemble
    disassembleChunk( &chunk, "test chunk" );

    // done
    freeChunk( &chunk );
    return 0;
}
