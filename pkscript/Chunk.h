#pragma once
#include "pkscript.h"
#include "Value.h"

#include <stdint.h>
#include <vector>

#define BYTE_MASK 0x000000FF;

struct Chunk
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
    OP_DEF_GLOBAL_SHORT,
    OP_DEF_GLOBAL,
    OP_DEF_GLOBAL_LONG,
    OP_GET_GLOBAL_SHORT,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL_SHORT,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_GET_LOCAL_SHORT,
    OP_GET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL_SHORT,
    OP_SET_LOCAL,
    OP_SET_LOCAL_LONG,
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
    OP_JUMP,
    OP_JUMP_BACK,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    //OP_GREATER_EQUAL,
    //OP_LESS_EQUAL,
    //OP_CONSTANT_LONG_LONG, //add to support 64-byte index locations, highly unlikely this will ever be needed
    OP_PRINT,
    OP_POP,
    OP_RETURN,
};

uint32_t addConstant(Chunk* chunk, Value value);

void writeChunk(Chunk* chunk, uint8_t byte, int line);

uint32_t writeConstant(Chunk* chunk, Value value, int line);

int getLine(Chunk* chunk, size_t offset);