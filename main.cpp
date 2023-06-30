#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

class Lexer
{
public:
    static char lastchar;
    static int numvalue;              // Filled in if tok_number
    static std::string identifierstr; // filled in if tok_identifier

    enum Token
    {
        tok_EOF = -1,
        tok_function = -2,
        tok_number = -3,
        tok_return = -4,
        tok_identifier = -5
    };

    static int getnext(std::string &s)
    {
        if (s.length() > 0)
        {
            char c = s.at(0);
            s = s.substr(1, s.length() - 1);
            return c;
        }
        return EOF;
    }

    static int get_token(std::string &content)
    {
        Lexer::lastchar = ' ';

        while (isspace(lastchar))
        {
            lastchar = getnext(content);
        }

        if (isalpha(lastchar))
            return is_identifier(content);

        if (isdigit(lastchar))
            return is_numeric(content);
        if (lastchar == EOF)
            return tok_EOF;

        return lastchar;
    }

    static int is_identifier(std::string &content)
    {
        identifierstr = lastchar;
        while (isalnum(lastchar))
        {
            if (isalnum(content.at(0)))
            {
                lastchar = getnext(content);
                identifierstr += lastchar;
            }
            else break;
        }
        if (identifierstr == "def")
        {
            return tok_function;
        }
        return tok_identifier;
    }

    static int is_numeric(std::string &content)
    {
        std::string num(1, lastchar);
        while (isdigit(lastchar))
        {
            if (content.length() > 0 && isalnum(content.at(0)))
            {
                lastchar = getnext(content);
                num += lastchar;
            }
            else
                break;
        }

        Lexer::numvalue = std::stoi(num); // Support only integers so far
        return tok_number;
    }
};
std::string Lexer::identifierstr;
int Lexer::numvalue;
char Lexer::lastchar;

static int current_token;
int getNextToken(std::string &line)
{
    current_token = Lexer::get_token(line);
    return current_token;
}

class ExprAST
{
public:
    virtual Value *codegen() = 0;
};
typedef std::vector<std::unique_ptr<ExprAST>> ExpressionList;

class PrototypeAST
{
    std::string name;
    std::vector<std::string> args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> Args)
    {
        this->name = name;
        this->args = std::move(Args);
    }
    Function *codegen();
};

class FunctionAST
{
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body; // body is collection of expressions

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
    {
        this->proto = std::move(Proto);
        this->body = std::move(Body);
        // for (auto &&expr : Body)
        // {
        //     this->body = std::move(expr);
        // }
    }
    Function *codegen();
};

class CallExprAST : public ExprAST
{
    std::string callee;
    ExpressionList args;

public:
    CallExprAST(const std::string &callee, ExpressionList args)
    {
        this->callee = callee;
        this->args = std::move(args);
    }
    Value *codegen() override;
};

class NumberExprAST : public ExprAST
{
    int value;
   
public:
    NumberExprAST(int value)
    {
        this->value = value;
    }
    Value* codegen() override;
};

static std::unique_ptr<ExprAST> ParsePrimary(std::string &content);
static std::unique_ptr<PrototypeAST> ParsePrototype(std::string &content)
{
    std::string FnName = Lexer::identifierstr;
    getNextToken(content);
    std::vector<std::string> ArgNames;
    while (getNextToken(content) == Lexer::tok_identifier)
        ArgNames.push_back(Lexer::identifierstr);
    getNextToken(content);
    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}
static std::unique_ptr<FunctionAST> ParseDefinition(std::string &content)
{
    getNextToken(content); // eat function keyword.
    auto Proto = ParsePrototype(content);
    auto body_block = ParsePrimary(content);
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(body_block));
}

static std::unique_ptr<ExprAST> ParseNumberExpr(std::string &line)
{
    auto result = std::make_unique<NumberExprAST>(Lexer::numvalue);
    getNextToken(line);
    return std::move(result);
}
static std::unique_ptr<ExprAST> ParseIdentifierExpr(std::string &line)
{
    // ADD TYPE NAMES!
    std::string id_name = Lexer::identifierstr;
    getNextToken(line); // skip function identifier
    getNextToken(line);
    std::vector<std::unique_ptr<ExprAST>> args; // function args
    std::unique_ptr<ExprAST> arg;

    while (true)
    {
        arg = ParsePrimary(line);
        if (arg)
        {
            args.push_back(std::move(arg));
        }
        else
        {
            getNextToken(line);
            return nullptr;
        }

        if (current_token == ')')
            break; // Done parsing args
        getNextToken(line);
    }

    getNextToken(line);

    return std::make_unique<CallExprAST>(id_name, std::move(args));
}
static std::unique_ptr<ExprAST> ParsePrimary(std::string &content)
{
    switch (current_token)
    {
    case Lexer::tok_identifier:
        return ParseIdentifierExpr(content);
    case Lexer::tok_number:
        return ParseNumberExpr(content);
    default:
        return nullptr;
    }
}

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;

void InitializeModule()
{
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("MyCompiler", *TheContext);
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

Function *PrototypeAST::codegen()
{
    FunctionType *FT = FunctionType::get(Type::getInt32Ty(*TheContext), true);
    Function *F = Function::Create(FT, Function::ExternalLinkage, this->name, TheModule.get());
    return F;
}

Function *FunctionAST::codegen()
{
    Function *TheFunction = this->proto->codegen();
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);
    this->body->codegen();
    Builder->CreateRet(ConstantInt::get(Type::getInt32Ty(*TheContext), 0, false)); // return 0
    verifyFunction(*TheFunction);

    return TheFunction;
}

Value *NumberExprAST::codegen()
{
    return ConstantInt::get(Type::getInt32Ty(*TheContext), this->value, true);
}

Value *CallExprAST::codegen()
{
    // Look up the name in the global module table.
    Function *CalleeF = TheModule->getFunction(this->callee);

    std::vector<Value*> ArgsV;
    for (size_t i = 0; i != args.size(); i++)
    {
        ArgsV.push_back(args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

void GenerateObject(std::string target)
{
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        TheModule->setTargetTriple(target);
        std::string Error;
        auto Target = TargetRegistry::lookupTarget(target, Error);
        auto CPU = "generic";
        auto Features = "";

        TargetOptions opt;
        auto RM = Optional<Reloc::Model>();
        auto TargetMachine = Target->createTargetMachine(target, CPU, Features, opt, RM);

        TheModule->setDataLayout(TargetMachine->createDataLayout());
        std::error_code EC;
        raw_fd_ostream dest("a.o", EC, sys::fs::OF_None);
        legacy::PassManager pass;
        auto FileType = CGFT_ObjectFile;

        TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType);
        pass.run(*TheModule);
        dest.flush();
}

void MainLoop(std::string &content)
{
    getNextToken(content);
    while (1)
    {
        switch (current_token)
        {
        case Lexer::tok_EOF:
            return;
        case Lexer::tok_function:
            auto F = ParseDefinition(content);
            auto IR = F->codegen();
            IR->print(errs());
            break;
        }
    }
}

Function* AddExternalFunc(std::string name) {
    FunctionType* FuncType = FunctionType::get(Type::getInt32Ty(*TheContext), {Type::getInt32Ty(*TheContext)}, false); // signature is: int f(int) 
    return Function::Create(FuncType, Function::ExternalLinkage, name, TheModule.get()); // add function to global module table
}

// create actual print function
void CreatePrintFunction()
{
    Function* PrintfFunc = AddExternalFunc("printf");
    Function* PrintFunc = AddExternalFunc("print");

    BasicBlock *entryBlock = BasicBlock::Create(*TheContext, "entry", PrintFunc);
    Builder->SetInsertPoint(entryBlock);
    Constant* formatStr = Builder->CreateGlobalStringPtr("%d\n");
    Builder->CreateCall(PrintfFunc, {formatStr, &*PrintFunc->arg_begin()});
    Builder->CreateRet(ConstantInt::get(Type::getInt32Ty(*TheContext), 0, false)); // return 0
} 

int main()
{
    InitializeModule();
    CreatePrintFunction();
    std::string content = "def main()\nprint(123)";
    MainLoop(content);
    GenerateObject("x86_64-pc-windows");
}