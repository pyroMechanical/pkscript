#include "pkscript.h"

#include "Block.h"
#include "Debug.h"
#include "VM.h"

static void repl(VM* vm)
{
    std::cout << "type 'exit' to exit REPL\n";
    std::string inputLine;
    do
    {
        std::getline(std::cin, inputLine);

        interpret(vm, inputLine.c_str());
    } while (inputLine != "exit");
}

static std::string readFile(std::string path)
{
    std::string source;
    std::ifstream in(path, std::ios::in | std::ios::ate);
    if (in)
    {
        source.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&source[0], source.size());
        in.close();
    }
    else
    {
        std::cerr << "Could not open file " << path << "." << std::endl;
        exit(74);
    }
    return source;
}

static void runFile(VM* vm, std::string path)
{
    std::string source = readFile(path);
    InterpretResult result = interpret(vm, source.c_str());

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}



int main(int argc, const char* argv[])
{
    VM vm = createVM();
    if(argc == 1)
    {
        repl(&vm);
    }
    else if (argc == 2)
    {
        runFile(&vm, std::string(argv[1]));
    }
    else
    {
        std::cerr << "Usage: pkscript [path]\n" << std::endl;
        exit(64);
    }
    freeVM(&vm);
}

