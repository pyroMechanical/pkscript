#pragma once

#include "Block.h"
#include <unordered_map>
#include <unordered_set>

struct VM
{
	Block* block;
	uint8_t* ip;
	ValueArray stack;
	Obj* objects;
	std::unordered_map<uint64_t, ObjString*> strings;
};

enum InterpretResult : uint8_t
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

VM* currentVM();

VM createVM();

void freeVM(VM* vm);

InterpretResult interpret(VM* vm, Block* block);
InterpretResult interpret(VM* vm, const char* source);

InterpretResult run(VM* vm);

uint32_t readbytes(VM* vm, uint32_t bytes);