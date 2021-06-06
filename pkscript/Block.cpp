#include "Block.h"

void writeBlock(Block* block, uint8_t byte, int line)
{
	block->code.push_back(byte);
	if (block->lines.size() != 0 && line == block->lines.back().first)
		block->lines.back().second++;
	else
		block->lines.emplace_back(std::pair<int, int>( line, 1 ));
}

void writeConstant(Block* block, Value value, int line)
{
	block->constants.push_back(value);
	uint32_t index = block->constants.size() - 1;
	if (index < UINT8_MAX)
	{
		uint8_t byte3 = index & BYTE_MASK;
		writeBlock(block, OP_CONSTANT_SHORT, line);
		writeBlock(block, byte3, line);
	}
	else if (index >= 256 && index < UINT16_MAX)
	{
		uint8_t byte2 = (index >> 8) & BYTE_MASK;
		uint8_t byte3 = index & BYTE_MASK;
		writeBlock(block, OP_CONSTANT, line);
		writeBlock(block, byte2, line);
		writeBlock(block, byte3, line);
	}
	else if (index >= 65536 && index < UINT32_MAX)
	{
		uint8_t byte0 = (index >> 24) & BYTE_MASK;
		uint8_t byte1 = (index >> 16) & BYTE_MASK;
		uint8_t byte2 = (index >> 8) & BYTE_MASK;
		uint8_t byte3 = index & BYTE_MASK;
		writeBlock(block, OP_CONSTANT_LONG, line);
		writeBlock(block, byte0, line);
		writeBlock(block, byte1, line);
		writeBlock(block, byte2, line);
		writeBlock(block, byte3, line);
	}
	else
	{
		ERR("PKS only supports up to 2^32 constants!");
	}
		
}

int getLine(Block* block, size_t offset)
{
	size_t lineOffset = 0;
	for(auto linecount : block->lines)
	{
		auto line = linecount.first;
		auto count = linecount.second;
		lineOffset += count;

		if (offset < lineOffset)
			return line;
	}
	return -1; //invalid line!
}