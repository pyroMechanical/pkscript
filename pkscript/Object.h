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
};

struct ObjString
{
	Obj obj;
	int length;
	char* chars;
};

ObjString* copyString(const char* chars, int length);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars.c_str())