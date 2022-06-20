#pragma once
#include "chunk.h"

void disassembleChunk( Chunk* chunk );
size_t disassembleInstruction( Chunk* chunk, size_t offset );
