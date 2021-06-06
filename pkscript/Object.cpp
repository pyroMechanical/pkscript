#include "pkscript.h"
#include "Object.h"

static Obj* allocateObject(size_t size, ObjType type)
{
}


ObjString* copyString(const char* chars, int length)
{
	std::string objString;
	objString.resize(length);
	memcpy((char*)objString.data(), chars, length);

	//return allocateString(objString);

	return new ObjString;
}