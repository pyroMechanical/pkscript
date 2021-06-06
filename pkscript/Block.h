#pragma once
#include "pkscript.h"
#include "Value.h"

#include <stdint.h>
#include <vector>

#define BYTE_MASK 0x000000FF;

struct Block
{
    std::vector<uint8_t> code;
    std::vector<std::pair<int, int>> lines;
    ValueArray constants;
};

enum OpCode : uint8_t
{
    OP_CONSTANT_SHORT,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_NEGATE,
    OP_ADD,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    //OP_GREATER_EQUAL,
    //OP_LESS_EQUAL,
    //OP_CONSTANT_LONG_LONG, //add to support 64-byte index locations, highly unlikely this will ever be needed
    OP_RETURN,
};

int addConstant(Block* block, Value value);

void writeBlock(Block* block, uint8_t byte, int line);

void writeConstant(Block* block, Value value, int line);

int getLine(Block* block, size_t offset);