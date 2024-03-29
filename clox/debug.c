#include <stdio.h>
#include "debug.h"
#include "object.h"
#include "value.h"

static size_t simpleInstruction( const char* name, size_t offset ) {
    printf( "%s", name );
    return offset + 1;
}

static int jumpInstruction( const char* name, int sign, Chunk* chunk, int offset ) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf( "%s(%d->%d)", name, offset, offset + 3 + sign * jump );
    return offset + 3;
}

static int byteInstruction( const char* name, Chunk* chunk, int offset ) {
    // get the slot that the instruction refers to
    uint8_t slot = chunk->code[offset + 1];
    
    // tab over 16 spaces, then print slot index
    printf( "%s(%d)", name, slot );
    
    // advance code by two bytes
    return offset + 2; 
}

static size_t constantInstruction( const char* name, Chunk* chunk, size_t offset ) {
    // get constant index
    uint8_t constantIndex = chunk->code[offset + 1];
    
    // print constant index
    printf( "%s(", name );
    
    // print the constant value followed by a newline
    printValue( chunk->constants.values[constantIndex] );
    printf( "@%d)", constantIndex );

    // advance code by two bytes
    return offset + 2;
}

static size_t closureInstruction( const char* name, Chunk* chunk, size_t offset ) {
    // print the closure
    offset++;
    int8_t constant = chunk->code[offset++];
    printf( "OP_CLOSURE(" );
    printValue( chunk->constants.values[constant] );
    printf( "@%d)", constant );

    // print the upvalues
    ObjFunction* function = AS_FUNCTION( chunk->constants.values[constant] );
    for( int i = 0; i < function->upvalueCount; i++ ) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf( "\n%04zu | %s %04d", offset - 2, isLocal ? "local" : "upvalue", index );
    }
    return offset;
}

static int invokeInstruction( const char* name, Chunk* chunk, int offset ) {
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf( "%-16s (%d args) %4d '", name, argCount, constant );
    printValue( chunk->constants.values[constant] );
    printf( "'\n" );
    return offset + 3;
}

size_t disassembleInstruction( Chunk* chunk, size_t offset ) {
    // print instruction offset & line number
    printf( "%04zu %04d ", offset, chunk->lines[offset] );

    // get instruction
    uint8_t instruction = chunk->code[offset];

    // dispatch on instruction
    switch( instruction ) {
        case OP_CONSTANT:       return constantInstruction( "OP_CONSTANT", chunk, offset );
        case OP_NIL:            return simpleInstruction( "OP_NIL", offset );
        case OP_TRUE:           return simpleInstruction( "OP_TRUE", offset );
        case OP_FALSE:          return simpleInstruction( "OP_FALSE", offset );
        case OP_POP:            return simpleInstruction( "OP_POP", offset );
        case OP_DEFINE_GLOBAL:  return constantInstruction( "OP_DEFINE_GLOBAL", chunk, offset );
        case OP_GET_GLOBAL:     return constantInstruction( "OP_GET_GLOBAL", chunk, offset );
        case OP_SET_GLOBAL:     return constantInstruction( "OP_SET_GLOBAL", chunk, offset );
        case OP_GET_UPVALUE:    return byteInstruction( "OP_GET_UPVALUE", chunk, offset );
        case OP_SET_UPVALUE:    return byteInstruction( "OP_SET_UPVALUE", chunk, offset );
        case OP_GET_LOCAL:      return byteInstruction( "OP_GET_LOCAL", chunk, offset );
        case OP_SET_LOCAL:      return byteInstruction( "OP_SET_LOCAL", chunk, offset );
        case OP_GET_PROPERTY:   return constantInstruction( "OP_GET_PROPERTY", chunk, offset );
        case OP_SET_PROPERTY:   return constantInstruction( "OP_SET_PROPERTY", chunk, offset );
        case OP_EQUAL:          return simpleInstruction( "OP_EQUAL", offset );
        case OP_GREATER:        return simpleInstruction( "OP_GREATER", offset );
        case OP_LESS:           return simpleInstruction( "OP_LESS", offset );
        case OP_ADD:            return simpleInstruction( "OP_ADD", offset );
        case OP_SUBTRACT:       return simpleInstruction( "OP_SUBTRACT", offset );
        case OP_MULTIPLY:       return simpleInstruction( "OP_MULTIPLY", offset );
        case OP_DIVIDE:         return simpleInstruction( "OP_DIVIDE", offset );
        case OP_NOT:            return simpleInstruction( "OP_NOT", offset );
        case OP_NEGATE:         return simpleInstruction( "OP_NEGATE", offset );
        case OP_PRINT:          return simpleInstruction( "OP_PRINT", offset );
        case OP_JUMP:           return jumpInstruction( "OP_JUMP", 1, chunk, offset );
        case OP_JUMP_IF_FALSE:  return jumpInstruction( "OP_JUMP_IF_FALSE", 1, chunk, offset );
        case OP_LOOP:           return jumpInstruction( "OP_LOOP", -1, chunk, offset );
        case OP_CALL:           return byteInstruction( "OP_CALL", chunk, offset );
        case OP_INVOKE:         return invokeInstruction( "OP_INVOKE", chunk, offset );
        case OP_CLOSURE:        return closureInstruction( "OP_CLOSURE", chunk, offset );
        case OP_CLOSE_UPVALUE:  return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN:         return simpleInstruction( "OP_RETURN", offset );
        case OP_CLASS:          return constantInstruction( "OP_CLASS", chunk, offset );
        case OP_METHOD:         return constantInstruction( "OP_METHOD", chunk, offset );
        case OP_INHERIT:        return simpleInstruction( "OP_INHERIT", offset );
        case OP_GET_SUPER:      return constantInstruction( "OP_GET_SUPER", chunk, offset );
        case OP_SUPER_INVOKE:   return invokeInstruction( "OP_SUPER_INVOKE", chunk, offset );
        default:
            printf( "Unknown opcode %d", instruction );
            return offset + 1;
    }
}

void disassembleChunk( Chunk* chunk ) {
    for( size_t offset = 0; offset < chunk->count; ) {
        offset = disassembleInstruction( chunk, offset );
        printf( "\n" );
    }
}
