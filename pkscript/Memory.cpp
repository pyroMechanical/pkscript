#include "pkscript.h"
#include "Memory.h"
#include "Object.h"
#include "VM.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
	if (newSize == 0)
	{
		free(pointer);
		return nullptr;
	}

	void* result = realloc(pointer, newSize);
	if (result == nullptr) exit(1);
	return result;
}

static void freeObject(Obj* object)
{
	switch(object->type)
	{
	case OBJ_STRING:
	{
		ObjString* string = (ObjString*)object;
		delete string;
	}
	}
}

void freeObjects()
{
	Obj* object = currentVM()->objects;

	while(object != nullptr)
	{
		Obj* next = object->next;
		freeObject(object);
		object = next;
	}
}