#include "pkscript.h"
#include "Object.h"
#include "Memory.h"
#include "VM.h"


static Obj* allocateObject(Obj* object, ObjType type)
{
	object->type = type;
	VM* vm = currentVM();
	object->next = vm->objects;
	vm->objects = object;
	return object;
}

static ObjString* allocateString(std::string chr_string)
{
	ObjString* stringObj = new ObjString(chr_string);
	allocateObject((Obj*)stringObj, OBJ_STRING);
	VM* vm = currentVM();
	vm->strings.emplace(std::make_pair(stringObj->string, stringObj));
	return stringObj;
}

ObjString* takeString(std::string chr_string)
{
	VM* vm = currentVM();
	auto val = vm->strings.find(chr_string);
	if (val != vm->strings.end())
		return val->second;
	return allocateString(chr_string);
}

ObjString* copyString(const char* chars, int length)
{
	std::string chr_string(chars, length);
	VM* vm = currentVM();
	auto val = vm->strings.find(chr_string);
	if (val != vm->strings.end())
		return val->second;
	return allocateString(chr_string);
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
	}
}