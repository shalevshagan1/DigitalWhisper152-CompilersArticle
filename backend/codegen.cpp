#include <vector>
#include <string>
#include <memory>
#include "types.h"
#include "codegen.h"

MOV::MOV(std::string dest, std::string source) {
    this->NAME = "MOV";
    this->params[0] = dest;
    this->params[1] = source;
}
std::string MOV::assemble() {
    return this->NAME + " " + this->params[0] + " " + this->params[1];
}

PUSHAD::PUSHAD()
{
    this->NAME = "PUSHAD";
}
std::string PUSHAD::assemble()
{
    return this->NAME;
}

POPAD::POPAD()
{
    this->NAME = "POPAD";
}
std::string POPAD::assemble() 
{
    return this->NAME;
}

INVOKE::INKOKE(std::string expression, std::vector<std::string> arguments) 
{
    this->NAME = "INVOKE";
    this->expression = expression;
    this->params = arguments;
}
std::string INVOKE::assemble() 
{
    return this->NAME + " " + this->expression + " " ;//+ this->params;
}


PROTOTYPE::PROTOTYPE(std::string label)
{
    this->label = label;
}
std::string PROTOTYPE::assemble() 
{
    return this->label + this->NAME ;//+ this->params;
}

void Generator::assemble_print(std::vector<std::unique_ptr<ExprAST>> args) 
{
    // add 3 function prototypes to headers
    this->dataseg += PROTOTYPE("GetStdHandle").assemble();
    this->dataseg += PROTOTYPE("WriteConsoleA").assemble();
    this->dataseg += PROTOTYPE("ExitProcess").assemble();
    
    // add INVOKE calls
    this->codeseg += INVOKE("GetStdHandle", std::vector<std::string>{"-11"}).assemble();
    this->codeseg += MOV("consoleOutHandle", "eax").assemble();
    this->codeseg += MOV("").assemble();
    this->codeseg += PUSHAD().assemble();
    this->codeseg += MOV().assemble();
    this->codeseg += INVOKE("WriteConsoleA", ...);
    this->codeseg += POPAD().assemble();
    this->codeseg += INVOKE("ExitProcess", 0);
}
void Generator::assemble_proc(std::string name, std::unique_ptr<ExprAST> expr)
{
    codeseg += name + " PROC";
    expr->codegen();
    codeseg += "END " + name;
}
void Generator::assemble_number(int value)
{
    this->dataseg += "rand_name DWORD {value}\n";
}
void Generator::output_all_asm()
{
    return this->processor + this->model + this->headers + this->dataseg + this->codeseg;
}

void PrototypeAST::codegen()
{

}

void FunctionAST::codegen() {

    // this->proto->codegen();
    this->assemble_proc("main", std::move(this->body));
    // return 
}

void NumberExprAST::codegen()
{
    gen.assemble_number(this->value); // return name
}

void CallExprAST::codegen()
{
    if (this->callee == "print")
    {
        gen.assemble_print(this->args); // add print to the assembly, it's an external function
    }
}

int main() 
{
    // PrototypeAST p("main", std::vector<std::string>());
    // FunctionAST f(std::move(p), std::make_unique<CallExprAST>("print", std::make_unique<NumberExprAST>(123)));

    // f->codegen();
}

// decide how to pack args
// What type should MOV get?