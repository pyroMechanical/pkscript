#pragma once

#include "Chunk.h"
#include <string>

void disassembleChunk(Chunk* chunk, const char* name);

size_t disassembleInstruction(Chunk* chunk, size_t offset);

static size_t simpleInstruction(const char* name, size_t offset);

static size_t constantInstruction(const char* name, Chunk* chunk, size_t offset, uint8_t bytes);