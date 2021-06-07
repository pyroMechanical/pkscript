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
	void (*prefix)();
	void (*infix)();
	Precedence precedence;
};

Parser parser;
Block* compilingBlock;

static Block* currentBlock()
{
	return compilingBlock;
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
	writeBlock(currentBlock(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void emitReturn()
{
	emitByte(OP_RETURN);
}

static void emitConstant(Value value)
{
	writeConstant(currentBlock(), value, parser.previous.line);
}

static void endCompiler()
{
	emitReturn();
#ifdef DEBUG_PRINT_CODE
	if(!parser.hadError)
	{
		disassembleBlock(currentBlock(), "code");
	}
#endif
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary()
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

static void literal()
{
	switch(parser.previous.type)
	{
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	default: return;
	}
}

static void grouping()
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number()
{
	double value = strtod(parser.previous.start, nullptr);
	emitConstant(createNumber(value));
}

static void string()
{
	emitConstant(createObject((Obj*)copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void unary()
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
	{grouping,    nullptr,   PREC_NONE},  //[TOKEN_LEFT_PAREN]
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_RIGHT_PAREN]
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_LEFT_BRACE]
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_RIGHT_BRACE]
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_COMMA]
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_DOT]
	{unary,        binary,   PREC_TERM},  //[TOKEN_MINUS]      
	{nullptr,      binary,   PREC_TERM},  //[TOKEN_PLUS]       
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_SEMICOLON]  
	{nullptr,      binary,   PREC_FACTOR},//[TOKEN_SLASH]      
	{nullptr,      binary,   PREC_FACTOR},//[TOKEN_STAR]       
	{unary,       nullptr,   PREC_NONE},  //[TOKEN_BANG]       
	{nullptr,     binary,    PREC_EQUALITY},  //[TOKEN_BANG_EQUAL] 
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_EQUAL]      
	{nullptr,     binary,    PREC_EQUALITY},  //[TOKEN_EQUAL_EQUAL]
	{nullptr,     binary,    PREC_COMPARISON},  //[TOKEN_GREATER]    
	{nullptr,     binary,    PREC_COMPARISON},  //[TOKEN_GREATER_EQUAL]
	{nullptr,     binary,    PREC_COMPARISON},  //[TOKEN_LESS]       
	{nullptr,     binary,    PREC_COMPARISON},  //[TOKEN_LESS_EQUAL] 
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_IDENTIFIER] 
	{string,     nullptr,    PREC_NONE},  //[TOKEN_STRING]     
	{number,      nullptr,   PREC_NONE},  //[TOKEN_NUMBER]     
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_AND]        
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_CLASS]      
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_ELSE]       
	{literal,     nullptr,   PREC_NONE},  //[TOKEN_FALSE]      
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_FOR]        
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_FUN]        
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_IF]         
	{literal,     nullptr,   PREC_NONE},  //[TOKEN_NIL]        
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_OR]         
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_PRINT]      
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_RETURN]     
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_SUPER]      
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_THIS]       
	{literal,     nullptr,   PREC_NONE},  //[TOKEN_TRUE]       
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_VAR]        
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_WHILE]      
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_ERROR]   
	{nullptr,     nullptr,   PREC_NONE},  //[TOKEN_EOF]    
};

static void parsePrecedence(Precedence precedence)
{
	advance();
	void (*prefixRule)() = getRule(parser.previous.type)->prefix;
	if (prefixRule == nullptr)
	{
		error("Expect expression.");
		return;
	}

	prefixRule();

	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		void (*infixRule)() = getRule(parser.previous.type)->infix;
		infixRule();
	}
}

static uint32_t identifierConstant(Token* name)
{
	return makeConstant(createObject((ObjString*)copyString(name->start, name->length)));
}

static uint32_t parseVariable(const char* errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);
	return identifierConstant(&parser.previous);
}

static ParseRule* getRule(TokenType type)
{
	return &rules[type];
}

static void expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
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
	defineVariable(global);
}

static void expressionStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

static void printStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
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
	if(match(TOKEN_PRINT))
	{
		printStatement();
	}
	else
	{
		expressionStatement();
	}
}

bool compile(VM* vm, const char* source, Block* block)
{
	initScanner(source);
	compilingBlock = block;

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