#pragma once
#include "pkscript.h"
#include <vector>

enum ValueType
{
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
};

struct Obj;

struct ObjString;

union _AS
{
	bool boolean;
	double number;
	Obj* obj;
};

struct Value
{
	ValueType type;
	_AS as;
};

#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

Value createBool(bool value);
Value createNil();
Value createNumber(double value);
Value createObject(Obj* value);

bool isFalsey(Value& value);
bool valuesEqual(Value a, Value b);


using ValueArray = std::vector<Value>;

void printValue(Value value);