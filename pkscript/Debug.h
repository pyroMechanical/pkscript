#pragma once

#include "Block.h"
#include <string>

void disassembleBlock(Block* block, const char* name);

size_t disassembleInstruction(Block* block, size_t offset);

static size_t simpleInstruction(const char* name, size_t offset);

static size_t constantInstruction(const char* name, Block* block, size_t offset, uint8_t bytes);