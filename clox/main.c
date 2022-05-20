#include <stdio.h>
#include "chunk.h"
#include "debug.h"

int main( __attribute__((unused)) int argc, __attribute__((unused)) const char* argv[] ) {
    Chunk chunk;
    initChunk( &chunk );
    writeChunk( &chunk, OP_RETURN );
    disassembleChunk( &chunk, "test chunk" );
    freeChunk( &chunk );
    return 0;
}
