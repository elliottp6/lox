#include <stdio.h>
#include "debug.h"

static size_t simpleInstruction( const char* name, size_t offset );

void disassembleChunk( Chunk* chunk, const char* name ) {
    // print chunk name
    printf( "== %s ==\n", name );

    // disassemble instructions one-at-a-time
    for( size_t offset = 0; offset < chunk->count; )
        offset = disassembleInstruction( chunk, offset );
}

size_t disassembleInstruction( Chunk* chunk, size_t offset ) {
    // print instruction offset (padded with up to 4 leading zeros)
    printf( "%04zu ", offset );

    // get instruction
    uint8_t instruction = chunk->code[offset];

    // dispatch on instruction
    switch( instruction ) {
        case OP_RETURN: return simpleInstruction( "OP_RETURN", offset );
        default: printf( "Unknown opcode %d\n", instruction ); return offset + 1;
    }
}

static size_t simpleInstruction( const char* name, size_t offset ) {
    printf( "%s\n", name );
    return offset + 1;
}
