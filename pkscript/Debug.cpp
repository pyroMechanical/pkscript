#include "pkscript.h"
#include "Debug.h"

void disassembleChunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    for (size_t offset = 0; offset < chunk->code.size();)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

size_t disassembleInstruction(Chunk* chunk, size_t offset)
{
    printf("%04d ", offset);

    if (offset > 0 && getLine(chunk, offset) == getLine(chunk, offset -1 ))
        printf("    | ");
    else
        printf("%4d ", getLine(chunk, offset));

    uint8_t instruction = chunk->code[offset];
    uint8_t byteLength = 0;
    switch (instruction)
    {
    case OP_NEGATE: return simpleInstruction("OP_NEGATE", offset);
    case OP_NOT: return simpleInstruction("OP_NOT", offset);
    case OP_NIL: return simpleInstruction("OP_NIL", offset);
    case OP_TRUE: return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE: return simpleInstruction("OP_FALSE", offset);
    case OP_POP: return simpleInstruction("OP_POP", offset);
    case OP_EQUAL: return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER: return simpleInstruction("OP_GREATER", offset);
    case OP_LESS: return simpleInstruction("OP_LESS", offset);
    case OP_ADD: return simpleInstruction("OP_ADD", offset);
    case OP_MULTIPLY: return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE: return simpleInstruction("OP_DIVIDE", offset);
    case OP_PRINT: return simpleInstruction("OP_PRINT", offset);
    case OP_JUMP: return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_BACK: return jumpInstruction("OP_JUMP_BACK", -1, chunk, offset);
    case OP_JUMP_IF_TRUE: return jumpInstruction("OP_JUMP_IF_TRUE", 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_RETURN: return simpleInstruction("OP_RETURN", offset);
    case OP_CONSTANT_SHORT: byteLength = 1; return constantInstruction("OP_CONSTANT_SHORT", chunk, offset, byteLength);
    case OP_CONSTANT: byteLength = 2; return constantInstruction("OP_CONSTANT", chunk, offset, byteLength);
    case OP_CONSTANT_LONG: byteLength = 4; return constantInstruction("OP_CONSTANT_LONG", chunk, offset, byteLength);
    case OP_DEF_GLOBAL_SHORT: byteLength = 1; return constantInstruction("OP_GLOBAL_SHORT", chunk, offset, byteLength);
    case OP_DEF_GLOBAL: byteLength = 2; return constantInstruction("OP_GLOBAL", chunk, offset, byteLength);
    case OP_DEF_GLOBAL_LONG: byteLength = 4; return constantInstruction("OP_GLOBAL_LONG", chunk, offset, byteLength);
    case OP_GET_GLOBAL_SHORT: byteLength = 1; return constantInstruction("OP_GET_GLOBAL_SHORT", chunk, offset, byteLength);
    case OP_GET_GLOBAL: byteLength = 2; return constantInstruction("OP_GET_GLOBAL", chunk, offset, byteLength);
    case OP_GET_GLOBAL_LONG: byteLength = 4; return constantInstruction("OP_GET_GLOBAL_LONG", chunk, offset, byteLength);
    case OP_SET_GLOBAL_SHORT: byteLength = 1; return constantInstruction("OP_SET_GLOBAL_SHORT", chunk, offset, byteLength);
    case OP_SET_GLOBAL: byteLength = 2; return constantInstruction("OP_SET_GLOBAL", chunk, offset, byteLength);
    case OP_SET_GLOBAL_LONG: byteLength = 4; return constantInstruction("OP_SET_GLOBAL_LONG", chunk, offset, byteLength);
    case OP_GET_LOCAL_SHORT: byteLength = 1; return constantInstruction("OP_GET_LOCAL_SHORT", chunk, offset, byteLength);
    case OP_GET_LOCAL: byteLength = 2; return constantInstruction("OP_GET_LOCAL", chunk, offset, byteLength);
    case OP_GET_LOCAL_LONG: byteLength = 4; return constantInstruction("OP_GET_LOCAL_LONG", chunk, offset, byteLength);
    case OP_SET_LOCAL_SHORT: byteLength = 1; return constantInstruction("OP_SET_LOCAL_SHORT", chunk, offset, byteLength);
    case OP_SET_LOCAL: byteLength = 2; return constantInstruction("OP_SET_LOCAL", chunk, offset, byteLength);
    case OP_SET_LOCAL_LONG: byteLength = 4; return constantInstruction("OP_SET_LOCAL_LONG", chunk, offset, byteLength);
    default:
        printf("Unknown Opcode %d\n", instruction);
        return offset + 1;
    }
}

static size_t simpleInstruction(const char* name, size_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static size_t jumpInstruction(const char* name, int sign, Chunk* chunk, size_t offset)
{
    uint16_t jump = (uint16_t)chunk->code[offset + 1];
    jump = (jump << 8) + chunk->code[offset + 2];
    printf("%-16s %04d -> %04d\n", name, (int)offset, (int)(offset + 3 + sign * jump));
    return offset + 3;
}

static size_t constantInstruction(const char* name, Chunk* chunk, size_t offset, uint8_t bytes)
{
    uint32_t constant = chunk->code[offset + 1];
    if (bytes > 1)
    {
        constant = (constant << 8) + chunk->code[offset + 2];
        if(bytes == 4)
        {
            constant = (constant << 8) + chunk->code[offset + 3];
            constant = (constant << 8) + chunk->code[offset + 4];
        }
    }
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants[constant]);
    printf("'\n");
    return offset + 1 + bytes;
}