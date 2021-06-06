#pragma once

#include "Block.h"
struct VM
{
	Block* block;
	uint8_t* ip;
	ValueArray stack;
};

enum InterpretResult : uint8_t
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

VM createVM();

void freeVM(VM* vm);

InterpretResult interpret(VM* vm, Block* block);
InterpretResult interpret(VM* vm, const char* source);

InterpretResult run(VM* vm);

uint32_t readbytes(VM* vm, uint32_t bytes);