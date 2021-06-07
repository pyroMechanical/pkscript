#include "pkscript.h"
#include "VM.h"
#include "Compiler.h"
#include "Memory.h"
#include "Debug.h"
#include "Object.h"

#include <stdarg.h>

VM* activeVM;

VM* currentVM()
{
	return activeVM;
}

VM createVM()
{
	VM vm;
	vm.stack.clear();
	vm.strings.clear();
	vm.objects = nullptr;
	return vm;
}

void freeVM(VM* vm)
{
	freeObjects();
}

static void runtimeError(VM* vm, const char* format...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = vm->ip - vm->block->code.data() - 1;
	int line = getLine(vm->block, instruction);

	fprintf(stderr, "[line &d] in script\n", line);
	vm->stack.clear();
}

InterpretResult interpret(VM* vm, const char* source)
{
	activeVM = vm;
	Block block;
	if (!compile(vm, source, &block))
	{
		return INTERPRET_COMPILE_ERROR;
	}

	vm->block = &block;
	vm->ip = &vm->block->code[0];

	InterpretResult result = run(vm);
	return INTERPRET_OK;
}

static Value popStack(VM* vm)
{
	Value val = vm->stack.back();
	vm->stack.pop_back();
	return val;
}

static Value peek(VM* vm, int distance)
{
	return vm->stack[vm->stack.size() -1 - distance];
}

static void concatenate(VM* vm)
{
	ObjString* b = AS_STRING(vm->stack.back());
	vm->stack.pop_back();
	ObjString* a = AS_STRING(vm->stack.back());
	vm->stack.pop_back();

	std::string chr_string;
	chr_string += a->string;
	chr_string += b->string;

	ObjString* result = takeString(chr_string);
	vm->stack.push_back(createObject((Obj*)result));
}

static InterpretResult run(VM* vm)
{
#define READ_CONSTANT(bytes) ( vm->block->constants[readbytes(vm, bytes)])
#define BINARY_OP(valueType, op) \
do { \
	if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) \
	{ \
 		runtimeError(vm, "Operands must be numbers."); \
		return INTERPRET_RUNTIME_ERROR; \
	} \
	double b = AS_NUMBER(popStack(vm)); \
	double a = AS_NUMBER(popStack(vm)); \
	vm->stack.push_back(valueType(a op b)); \
} while (false)


	for (;;)
	{
#ifdef DEBUG_TRACE_EXECUTION
		printf("        ");
		for (Value value : vm->stack)
		{
			printf("[");
			printValue(value);
			printf("]");
		}
		printf("\n");
		disassembleInstruction(vm->block, (size_t) (vm->ip - &(vm->block->code[0])));
#endif
		uint8_t instruction = readbytes(vm, 1);
		switch (instruction)
		{
			case OP_CONSTANT_SHORT:
			{
				Value constant = READ_CONSTANT(1);
				vm->stack.push_back(constant);
				break;
			}
			case OP_CONSTANT:
			{
				Value constant = READ_CONSTANT(2);
				vm->stack.push_back(constant);
				break;
			}
			case OP_CONSTANT_LONG:
			{
				Value constant = READ_CONSTANT(4);
				vm->stack.push_back(constant);
				break;
			}
			case OP_FALSE: vm->stack.push_back(createBool(false)); break;
			case OP_TRUE: vm->stack.push_back(createBool(true)); break;
			case OP_NIL: vm->stack.push_back(createNil()); break;
			case OP_NOT: vm->stack.back() = createBool(isFalsey(vm->stack.back())); break;
			case OP_POP: popStack(vm); break;
			case OP_EQUAL:
				Value b = popStack(vm);
				Value a = popStack(vm);
				vm->stack.push_back(createBool(valuesEqual(a, b)));
				break;
			case OP_GREATER: BINARY_OP(createBool, > ); break;
			case OP_LESS: BINARY_OP(createBool, < ); break;
			case OP_NEGATE:
			{
				if(!IS_NUMBER(peek(vm, 0)))
				{
					runtimeError(vm, "Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				Value& n = vm->stack.back();
				n.as.number = -n.as.number;
				break;
			}
			case OP_ADD:
			{
				if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1)))
				{
					concatenate(vm);
				}
				else if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 0)))
				{
					double b = AS_NUMBER(popStack(vm));
					double a = AS_NUMBER(popStack(vm));

					vm->stack.push_back(createNumber(a + b));
				}
				else
				{
					runtimeError(vm, "Operands must be two numbers or two strings.");
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}
			case OP_MULTIPLY: BINARY_OP(createNumber, *); break;
			case OP_DIVIDE: BINARY_OP(createNumber, /); break;
			case OP_PRINT: printValue(popStack(vm)); printf("\n"); break;
			case OP_RETURN:
			{
				// Exit interpreter
				return INTERPRET_OK;
			}
		}
	}

#undef READ_CONSTANT
#undef BINARY_OP
}

uint32_t readbytes(VM* vm, uint32_t bytes)
{
	uint32_t out = 0;
	for (uint32_t i = 0; i < bytes; i++)
	{
		out = out << 8 | *vm->ip++;
	}
	return out;
}

