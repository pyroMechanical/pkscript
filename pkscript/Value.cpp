#include "Value.h"

void printValue(Value value)
{

	switch (value.type)
	{
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	case VAL_NUMBER: printf("%g", AS_NUMBER(value));
	}
}

Value createBool(bool value)
{
	Value result = {};
	result.type = VAL_BOOL;
	result.as.boolean = value;
	return result;
}

Value createNil()
{
	Value result = {};
	result.type = VAL_NIL;
	result.as.number = 0;
	return result;
}

Value createNumber(double value)
{
	Value result = {};
	result.type = VAL_NUMBER;
	result.as.number = value;
	return result;
}

Value createObject(Obj* value)
{
	Value result = {};
	result.type = VAL_OBJ;
	result.as.obj = value;
	return result;
}

bool isFalsey(Value& value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool valuesEqual(Value a, Value b)
{
	if (a.type != b.type) return false;
	switch(a.type)
	{
	case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL: return true;
	case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
	default: return false;
	}
}