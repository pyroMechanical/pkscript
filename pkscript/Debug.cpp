#include "pkscript.h"
#include "Debug.h"

void disassembleBlock(Block* block, const char* name)
{
    printf("== %s ==\n", name);
    for (size_t offset = 0; offset < block->code.size();)
    {
        offset = disassembleInstruction(block, offset);
    }
}

size_t disassembleInstruction(Block* block, size_t offset)
{
    printf("%04d ", offset);

    if (offset > 0 && getLine(block, offset) == getLine(block, offset -1 ))
        printf("    | ");
    else
        printf("%4d ", getLine(block, offset));

    uint8_t instruction = block->code[offset];
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
    case OP_RETURN: return simpleInstruction("OP_RETURN", offset);
    case OP_CONSTANT_SHORT: byteLength = 1; return constantInstruction("OP_CONSTANT_SHORT", block, offset, byteLength);
    case OP_CONSTANT: byteLength = 2; return constantInstruction("OP_CONSTANT", block, offset, byteLength);
    case OP_CONSTANT_LONG: byteLength = 4; return constantInstruction("OP_CONSTANT_LONG", block, offset, byteLength);
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

static size_t constantInstruction(const char* name, Block* block, size_t offset, uint8_t bytes)
{
    uint32_t constant = block->code[offset + 1];
    if (bytes > 1)
    {
        constant = (constant << 8) + block->code[offset + 2];
        if(bytes == 4)
        {
            constant = (constant << 8) + block->code[offset + 3];
            constant = (constant << 8) + block->code[offset + 4];
        }
    }
    printf("%-16s %4d '", name, constant);
    printValue(block->constants[constant]);
    printf("'\n");
    return offset + 1 + bytes;
}