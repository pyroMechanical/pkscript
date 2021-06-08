#include "Chunk.h"

void writeChunk(Chunk* chunk, uint8_t byte, int line)
{
	chunk->code.push_back(byte);
	if (chunk->lines.size() != 0 && line == chunk->lines.back().first)
		chunk->lines.back().second++;
	else
		chunk->lines.emplace_back(std::pair<int, int>( line, 1 ));
}

uint32_t addConstant(Chunk* chunk, Value value)
{
	chunk->constants.push_back(value);
	return chunk->constants.size() - 1;
}

uint32_t writeConstant(Chunk* chunk, Value value, int line)
{
		uint32_t index = addConstant(chunk, value);
		if (index < UINT8_MAX)
		{
			uint8_t byte3 = index & BYTE_MASK;
			writeChunk(chunk, OP_CONSTANT_SHORT, line);
			writeChunk(chunk, byte3, line);
		}
		else if (index > UINT8_MAX && index < UINT16_MAX)
		{
			uint8_t byte2 = (index >> 8) & BYTE_MASK;
			uint8_t byte3 = index & BYTE_MASK;
			writeChunk(chunk, OP_CONSTANT, line);
			writeChunk(chunk, byte2, line);
			writeChunk(chunk, byte3, line);
		}
		else if (index > UINT16_MAX && index < UINT32_MAX)
		{
			uint8_t byte0 = (index >> 24) & BYTE_MASK;
			uint8_t byte1 = (index >> 16) & BYTE_MASK;
			uint8_t byte2 = (index >> 8) & BYTE_MASK;
			uint8_t byte3 = index & BYTE_MASK;
			writeChunk(chunk, OP_CONSTANT_LONG, line);
			writeChunk(chunk, byte0, line);
			writeChunk(chunk, byte1, line);
			writeChunk(chunk, byte2, line);
			writeChunk(chunk, byte3, line);
		}
		else
		{
			ERR("PKS only supports up to 2^32 constants!");
		}
	return index;
}

int getLine(Chunk* chunk, size_t offset)
{
	size_t lineOffset = 0;
	for(auto linecount : chunk->lines)
	{
		auto line = linecount.first;
		auto count = linecount.second;
		lineOffset += count;

		if (offset < lineOffset)
			return line;
	}
	return -1; //invalid line!
}