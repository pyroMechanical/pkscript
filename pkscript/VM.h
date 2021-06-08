#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <unordered_set>

struct VM
{
	Chunk* chunk;
	uint8_t* ip;
	ValueArray stack;
	Obj* objects;
	std::unordered_map<std::string, Value> globals;
	std::unordered_map<std::string, ObjString*> strings;
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

InterpretResult interpret(VM* vm, Chunk* chunk);
InterpretResult interpret(VM* vm, const char* source);

InterpretResult run(VM* vm);

uint32_t readbytes(VM* vm, uint32_t bytes);