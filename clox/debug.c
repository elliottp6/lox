#include <stdio.h>
#include "debug.h"
#include "value.h"

static size_t simpleInstruction( const char* name, size_t offset ) {
    printf( "%s", name );
    return offset + 1;
}

static size_t constantInstruction( const char* name, Chunk* chunk, size_t offset ) {
    // get constant index
    uint8_t constantIndex = chunk->code[offset + 1];
    
    // tab over 16 spaces, then print constant index
    printf( "%s<", name );
    
    // print the constant value followed by a newline
    printValue( chunk->constants.values[constantIndex] );

    printf( "@%d>", constantIndex );

    // advance code by two bytes
    return offset + 2;
}

size_t disassembleInstruction( Chunk* chunk, size_t offset ) {
    // print instruction offset & line number
    printf( "%04zu %04d ", offset, chunk->lines[offset] );

    // get instruction
    uint8_t instruction = chunk->code[offset];

    // dispatch on instruction
    switch( instruction ) {
        case OP_CONSTANT:   return constantInstruction( "OP_CONSTANT", chunk, offset );
        case OP_NIL:        return simpleInstruction( "OP_NIL", offset );
        case OP_TRUE:       return simpleInstruction( "OP_TRUE", offset );
        case OP_FALSE:      return simpleInstruction( "OP_FALSE", offset );
        case OP_POP:        return simpleInstruction( "OP_POP", offset );
        case OP_DEFINE_GLOBAL: return constantInstruction( "OP_DEFINE_GLOBAL", chunk, offset );
        case OP_EQUAL:      return simpleInstruction( "OP_EQUAL", offset );
        case OP_GREATER:    return simpleInstruction( "OP_GREATER", offset );
        case OP_LESS:       return simpleInstruction( "OP_LESS", offset );
        case OP_ADD:        return simpleInstruction( "OP_ADD", offset );
        case OP_SUBTRACT:   return simpleInstruction( "OP_SUBTRACT", offset );
        case OP_MULTIPLY:   return simpleInstruction( "OP_MULTIPLY", offset );
        case OP_DIVIDE:     return simpleInstruction( "OP_DIVIDE", offset );
        case OP_NOT:        return simpleInstruction( "OP_NOT", offset );
        case OP_NEGATE:     return simpleInstruction( "OP_NEGATE", offset );
        case OP_PRINT:      return simpleInstruction( "OP_PRINT", offset );
        case OP_RETURN:     return simpleInstruction( "OP_RETURN", offset );
        default:
            printf( "Unknown opcode %d", instruction );
            return offset + 1;
    }
}

void disassembleChunk( Chunk* chunk, const char* name ) {
    // print chunk name
    printf( "== %s ==\n", name );

    // disassemble instructions one-at-a-time
    for( size_t offset = 0; offset < chunk->count; ) {
        offset = disassembleInstruction( chunk, offset );
        printf( "\n" );
    }
}
