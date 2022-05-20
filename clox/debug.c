#include <stdio.h>
#include "debug.h"
#include "value.h"

static size_t simpleInstruction( const char* name, size_t offset ) {
    printf( "%s\n", name );
    return offset + 1;
}

static size_t constantInstruction( const char* name, Chunk* chunk, size_t offset ) {
    // get constant index
    uint8_t constantIndex = chunk->code[offset + 1];
    
    // tab over 16 spaces, then print constant index
    printf( "%-16s %4d '", name, constantIndex );
    
    // print the constant value followed by a newline
    printValue( chunk->constants.values[constantIndex] );
    printf( "'\n" );

    // advance code by two bytes
    return offset + 2;
}

size_t disassembleInstruction( Chunk* chunk, size_t offset ) {
    // print instruction offset (padded with up to 4 leading zeros)
    printf( "%04zu ", offset );

    // print line number
    printf( "%4d ", chunk->lines[offset] );

    // get instruction
    uint8_t instruction = chunk->code[offset];

    // dispatch on instruction
    switch( instruction ) {
        case OP_CONSTANT: return constantInstruction( "OP_CONSTANT", chunk, offset );
        case OP_RETURN: return simpleInstruction( "OP_RETURN", offset );
        default: printf( "Unknown opcode %d\n", instruction ); return offset + 1;
    }
}

void disassembleChunk( Chunk* chunk, const char* name ) {
    // print chunk name
    printf( "== %s ==\n", name );

    // disassemble instructions one-at-a-time
    for( size_t offset = 0; offset < chunk->count; )
        offset = disassembleInstruction( chunk, offset );
}
