#include "pkscript.h"
#include "Compiler.h"
#include "Debug.h"
#include "Object.h"
#include "Scanner.h"

#include <array>

struct Parser
{
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
};

enum Precedence
{
	PREC_NONE,
	PREC_ASSIGNMENT, // = 
	PREC_OR, // or
	PREC_AND, // and
	PREC_EQUALITY, // ==, !=
	PREC_COMPARISON, // <, >, <=, >=
	PREC_TERM, // +, -
	PREC_FACTOR, // *, /
	PREC_UNARY, // !, -
	PREC_CALL, // ., ()
	PREC_PRIMARY
};

struct ParseRule
{
	void (*prefix)(bool);
	void (*infix)(bool);
	Precedence precedence;
};

struct Local
{
	Token name;
	int depth;

	Local(Token name, int depth)
		:name(name), depth(depth) {}
};


struct Compiler
{
	std::vector<Local> locals;
	int scopeDepth;
};


Parser parser;
Compiler* current = nullptr;
Chunk* compilingChunk;

static Chunk* currentChunk()
{
	return compilingChunk;
}

static void errorAt(Token* token, const char* message)
{
	if (parser.panicMode) return;
	parser.panicMode = true;
	std::cerr << token->line << " Error" << std::endl;

	if(token->type == TOKEN_EOF)
	{
		std::cerr << " at end";
	}
	else if (token->type == TOKEN_ERROR)
	{
		//nothing here!
	}
	else
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}


static void error(const char* message)
{
	errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message)
{
	errorAt(&parser.current, message);
}

static void advance()
{
	parser.previous = parser.current;

	for(;;)
	{
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR) break;
		errorAtCurrent(parser.current.start);
	}
}

static void consume(TokenType type, const char* message)
{
	if (parser.current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

static void emitByte(uint8_t byte)
{
	writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void emitLoop(size_t loopStart)
{
	emitByte(OP_JUMP_BACK);

	size_t offset = currentChunk()->code.size() - loopStart + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");

	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction)
{
	emitByte(instruction);
	emitBytes(0xff, 0xff);
	return currentChunk()->code.size() - 2;
}

static void emitReturn()
{
	emitByte(OP_RETURN);
}

static uint32_t emitConstant(Value value)
{
	return writeConstant(currentChunk(), value, parser.previous.line);
}

static void patchJump(int offset)
{
	int jump = currentChunk()->code.size() - offset - 2;

	if(jump > UINT16_MAX)
	{
		error("Jump offset too large, must be 65,535 or lower.");
	}

	currentChunk()->code[offset] = (jump >> 8) & BYTE_MASK;
	currentChunk()->code[(size_t)offset+1] = jump & BYTE_MASK;
}

static void initCompiler(Compiler* compiler)
{
	compiler->locals.clear();
	compiler->scopeDepth = 0;
	current = compiler;
}


static void endCompiler()
{
	emitReturn();
#ifdef DEBUG_PRINT_CODE
	if(!parser.hadError)
	{
		disassembleChunk(currentChunk(), "code");
	}
#endif
}

static void beginScope()
{
	current->scopeDepth++;
}

static void endScope()
{
	current->scopeDepth--;

	while(current->locals.size() > 0 && current->locals[current->locals.size() - 1].depth > current->scopeDepth)
	{
		emitByte(OP_POP);
		current->locals.pop_back();
	}
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static uint32_t identifierConstant(Token* name);
static void emitVariable(const char* type, uint32_t index, bool global = false);
static void and_(bool canAssign);
static void or_(bool canAssign);

static int resolveLocal(Compiler* compiler, Token* name);

static void binary(bool canAssign)
{
	TokenType operatorType = parser.previous.type;
	ParseRule* rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType)
	{
	case TOKEN_BANG_EQUAL:		emitByte(OP_EQUAL); 
								emitByte(OP_NOT); break;
	case TOKEN_EQUAL_EQUAL:		emitByte(OP_EQUAL); break;
	case TOKEN_GREATER:			emitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL:	emitByte(OP_LESS); 
								emitByte(OP_NOT); break;
	case TOKEN_LESS:			emitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL:      emitByte(OP_GREATER); 
								emitByte(OP_NOT); break;
	case TOKEN_PLUS:			emitByte(OP_ADD); break;
	case TOKEN_MINUS:			emitByte(OP_NEGATE); 
								emitByte(OP_ADD); break;
	case TOKEN_STAR:			emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:			emitByte(OP_DIVIDE); break;
	}
}

static void literal(bool canAssign)
{
	switch(parser.previous.type)
	{
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	default: return;
	}
}

static void grouping(bool canAssign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
	double value = strtod(parser.previous.start, nullptr);
	emitConstant(createNumber(value));
}

static void string(bool canAssign)
{
	emitConstant(createObject((Obj*)copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
	int arg = resolveLocal(current, &name);
	bool global;
	if(arg != -1)
	{
		global = false;
	}
	else
	{
		arg = identifierConstant(&name);
		global = true;
	}
	

	if(canAssign && match(TOKEN_EQUAL))
	{
		expression();
		emitVariable("set", arg, global);
	}
	else 
	{
		emitVariable("get", arg, global);
	}
}

static void variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign)
{
	TokenType operatorType = parser.previous.type;

	parsePrecedence(PREC_UNARY);

	switch(operatorType)
	{
	case TOKEN_BANG: emitByte(OP_NOT); break;
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	default: return;
	}
}

ParseRule rules[] = {
	{grouping,nullptr,       PREC_NONE},  //[TOKEN_LEFT_PAREN]
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_RIGHT_PAREN]
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_LEFT_BRACE]
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_RIGHT_BRACE]
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_COMMA]
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_DOT]
	{unary,    binary,       PREC_TERM},  //[TOKEN_MINUS]      
	{nullptr,  binary,       PREC_TERM},  //[TOKEN_PLUS]       
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_SEMICOLON]  
	{nullptr,  binary,     PREC_FACTOR},//[TOKEN_SLASH]      
	{nullptr,  binary,     PREC_FACTOR},//[TOKEN_STAR]       
	{unary,   nullptr,       PREC_NONE},  //[TOKEN_BANG]       
	{nullptr,  binary,   PREC_EQUALITY},  //[TOKEN_BANG_EQUAL] 
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_EQUAL]      
	{nullptr,  binary,   PREC_EQUALITY},  //[TOKEN_EQUAL_EQUAL]
	{nullptr,  binary, PREC_COMPARISON},  //[TOKEN_GREATER]    
	{nullptr,  binary, PREC_COMPARISON},  //[TOKEN_GREATER_EQUAL]
	{nullptr,  binary, PREC_COMPARISON},  //[TOKEN_LESS]       
	{nullptr,  binary, PREC_COMPARISON},  //[TOKEN_LESS_EQUAL] 
	{variable,nullptr,       PREC_NONE},  //[TOKEN_IDENTIFIER] 
	{string,  nullptr,       PREC_NONE},  //[TOKEN_STRING]     
	{number,  nullptr,       PREC_NONE},  //[TOKEN_NUMBER]     
	{nullptr,    and_,        PREC_AND},  //[TOKEN_AND]        
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_CLASS]      
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_ELSE]       
	{literal, nullptr,       PREC_NONE},  //[TOKEN_FALSE]      
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_FOR]        
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_FUN]        
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_IF]         
	{literal, nullptr,       PREC_NONE},  //[TOKEN_NIL]        
	{nullptr,     or_,         PREC_OR},  //[TOKEN_OR]         
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_PRINT]      
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_RETURN]     
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_SUPER]      
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_THIS]       
	{literal, nullptr,       PREC_NONE},  //[TOKEN_TRUE]       
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_VAR]        
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_WHILE]      
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_ERROR]   
	{nullptr, nullptr,       PREC_NONE},  //[TOKEN_EOF]    
};

static void parsePrecedence(Precedence precedence)
{
	advance();
	void (*prefixRule)(bool) = getRule(parser.previous.type)->prefix;
	if (prefixRule == nullptr)
	{
		error("Expect expression.");
		return;
	}

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		void (*infixRule)(bool) = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}
}

static uint32_t identifierConstant(Token* name)
{
	return addConstant(currentChunk(), createObject((Obj*)copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b)
{
	if (a->length != b->length) return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name)
{
	for(int i = current->locals.size() - 1; i >= 0; i--)
	{
		Local* local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name))
		{
			if(local->depth == -1)
			{
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

static void addLocal(Token name)
{
	current->locals.push_back({ name, -1 });
}

static void declareVariable()
{
	if (current->scopeDepth == 0) return;
	Token* name = &parser.previous;

	for(int i = current->locals.size()-1; i >=0; i--)
	{
		Local* local = &current->locals[i];
		if(local->depth != -1 && local->depth < current->scopeDepth)
		{
			break;
		}

		if(identifiersEqual(name, &local->name))
		{
			error("Already a variable with this name in this scope.");
		}

	}
	addLocal(*name);
}

static uint32_t parseVariable(const char* errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();
	if (current->scopeDepth > 0) return 0;

	return identifierConstant(&parser.previous);
}

static void markInitialized()
{
	current->locals[(size_t)current->locals.size() -1].depth = current->scopeDepth;
}

static void emitVariable(const char* type , uint32_t index, bool global)
{
	if (!global && current->scopeDepth > 0)
	{
		markInitialized();
	}
	if (index < UINT8_MAX)
	{
		uint8_t byte3 = index & BYTE_MASK;
		if (strcmp(type, "def") == 0)
		{
			if (global)
				emitByte(OP_DEF_GLOBAL_SHORT);
			else
				emitByte(OP_SET_LOCAL_SHORT);
		}
		else if (strcmp(type, "get") == 0)
		{
			if (global)
				emitByte(OP_GET_GLOBAL_SHORT);
			else
				emitByte(OP_GET_LOCAL_SHORT);
		}
		else if (strcmp(type, "set") == 0)
		{
			if (global)
				emitByte(OP_SET_GLOBAL_SHORT);
			else
				emitByte(OP_SET_LOCAL_SHORT);
		}
		emitByte(byte3);
	}
	else if (index > UINT8_MAX && index < UINT16_MAX)
	{
		uint8_t byte2 = (index >> 8) & BYTE_MASK;
		uint8_t byte3 = index & BYTE_MASK;
		if (strcmp(type, "def") == 0)
		{
			if (global)
				emitByte(OP_DEF_GLOBAL);
			else
				emitByte(OP_SET_LOCAL);
		}
		else if (strcmp(type, "get") == 0)
		{
			if (global)
				emitByte(OP_GET_GLOBAL);
			else
				emitByte(OP_GET_LOCAL);
		}
		else if (strcmp(type, "set") == 0)
		{
			if (global)
				emitByte(OP_SET_GLOBAL);
			else
				emitByte(OP_SET_LOCAL);
		}
		emitByte(byte2);
		emitByte(byte3);
	}
	else if (index > UINT16_MAX && index < UINT32_MAX)
	{
		uint8_t byte0 = (index >> 24) & BYTE_MASK;
		uint8_t byte1 = (index >> 16) & BYTE_MASK;
		uint8_t byte2 = (index >> 8) & BYTE_MASK;
		uint8_t byte3 = index & BYTE_MASK;
		if (strcmp(type, "def") == 0)
		{
			if (global)
				emitByte(OP_DEF_GLOBAL_LONG);
			else
				emitByte(OP_SET_LOCAL_LONG);
		}
		else if (strcmp(type, "get") == 0)
		{
			if (global)
				emitByte(OP_GET_GLOBAL_LONG);
			else
				emitByte(OP_GET_LOCAL_LONG);
		}
		else if (strcmp(type, "set") == 0)
		{
			if (global)
				emitByte(OP_SET_GLOBAL_LONG);
			else			
				emitByte(OP_SET_LOCAL_LONG);
		}
		emitByte(byte0);
		emitByte(byte1);
		emitByte(byte2);
		emitByte(byte3);
	}
}

static void and_(bool canAssign)
{
	int endJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

static void or_(bool canAssign)
{
	int elseJump = emitJump(OP_JUMP_IF_TRUE);

	emitByte(OP_POP);
	parsePrecedence(PREC_OR);

	patchJump(elseJump);
}

static ParseRule* getRule(TokenType type)
{
	return &rules[type];
}

static void expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void block()
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
	{
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration()
{
	uint32_t global = parseVariable("Expect Variable name.");

	if(match(TOKEN_EQUAL))
	{
		expression();
	}
	else
	{
		emitByte(OP_NIL);
	}
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	emitVariable("def", global, current->scopeDepth <= 0);
}

static void expressionStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

static void forStatement()
{
	beginScope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	if(match(TOKEN_SEMICOLON))
	{
		
	}
	else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		expressionStatement();
	}
	
	int loopStart = currentChunk()->code.size();
	int exitJump = -1;
	if(!match(TOKEN_SEMICOLON))
	{
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP);
	}
	if(!match(TOKEN_RIGHT_PAREN))
	{
		int bodyJump = emitJump(OP_JUMP);
		int incrementStart = currentChunk()->code.size();
		expression();
		emitByte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clause.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();
	emitLoop(loopStart);

	if (exitJump != -1)
	{
		patchJump(exitJump);
		emitByte(OP_POP);
	}
	endScope();
}

static void ifStatement()
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();

	int elseJump = emitJump(OP_JUMP);
	patchJump(thenJump);
	emitByte(OP_POP);
	if (match(TOKEN_ELSE)) statement();

	patchJump(elseJump);
}

static void printStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

static void whileStatement()
{
	size_t loopStart = currentChunk()->code.size();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	emitLoop(loopStart);

	patchJump(exitJump);
	emitByte(OP_POP);
}

static void synchronize()
{
	parser.panicMode = false;

	while(parser.current.type != TOKEN_EOF)
	{
		if (parser.previous.type == TOKEN_SEMICOLON) return;
		switch(parser.current.type)
		{
		case TOKEN_CLASS:
		case TOKEN_FUNC:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:;
		}

		advance();
	}
}

static void declaration()
{
	if(match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (parser.panicMode) synchronize();
}

static void statement()
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
	else if (match(TOKEN_FOR))
	{
		forStatement();
	}
	else if (match(TOKEN_IF))
	{
		ifStatement();
	}
	else if (match(TOKEN_WHILE))
	{
		whileStatement();
	}
	else if (match(TOKEN_LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else
	{
		expressionStatement();
	}
}

bool compile(VM* vm, const char* source, Chunk* chunk)
{
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler);
	compilingChunk = chunk;

	parser.hadError = false;
	parser.panicMode = false;

	advance();
	while (!match(TOKEN_EOF))
	{
		declaration();
	}
	consume(TOKEN_EOF, "Expect end of expression.");
	endCompiler();
	return !parser.hadError;
}