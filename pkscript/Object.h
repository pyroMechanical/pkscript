#pragma once

#include "pkscript.h"
#include "Value.h"

enum ObjType
{
	OBJ_STRING,
};

struct Obj
{
	ObjType type;
	Obj* next;
};

struct ObjString
{
	Obj obj;
	std::string string;
	size_t hash;

	ObjString(std::string chr_string)
		: string(chr_string) 
	{
		hash = std::hash<std::string>{}(string);
	}
};

ObjString* takeString(std::string chr_string);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->string.c_str())