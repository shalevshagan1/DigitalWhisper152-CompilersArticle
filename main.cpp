// #include <string>
// #include <vector>
// #include <memory>
// #include "llvm/IR/IRBuilder.h"
// #include "llvm/IR/Verifier.h"
// #include "llvm/IR/LegacyPassManager.h"
// #include "llvm/MC/TargetRegistry.h"
// #include "llvm/Support/FileSystem.h"
// #include "llvm/Support/Host.h"
// #include "llvm/Support/TargetSelect.h"
// #include "llvm/Support/raw_ostream.h"
// #include "llvm/Target/TargetMachine.h"
// #include "llvm/Target/TargetOptions.h"

class Lexer
{
public:
   static char lastchar;
   static int numvalue; // Filled in if tok_number

   enum Token
   {
       tok_EOF = -1,
       tok_function = -2,
       tok_number = -3,
       tok_return = -4
   };

   int get_token(std::string& content)
   {
       while (isspace(Lexer::lastchar))
       {
           Lexer::lastchar = getnext(content);
       }
       if (isalpha(Lexer::lastchar))
           return Lexer::is_identifier(content);

       if (isdigit(Lexer::lastchar))
           return Lexer::is_numeric(content);
       if (Lexer::lastchar == EOF)
           return tok_EOF;

       return Lexer::lastchar;
   }
};

class PrototypeAST
{
   std::string name;
   std::vector<std::string> args;

public:
   PrototypeAST(const std::string& name, std::vector<std::string> Args)
   {
       this->name = name;
       this->args = std::move(Args);
   }
};

class FunctionAST
{
   std::unique_ptr<PrototypeAST> proto;
   Block body; // body is collection of expressions

public:
   FunctionAST(std::unique_ptr<PrototypeAST> Proto, Block Body)
   {
       this->proto = std::move(Proto);
       for (auto&& expr : Body)
       {
           this->body.emplace_back(std::move(expr));
       }
   }
};

class ExprAST
{
public:
   virtual ~ExprAST() = 0;
};
typedef std::vector<std::unique_ptr<ExprAST>> Block;

class CallExprAST : public ExprAST
{
   std::string callee;
   Block args;

public:
   CallExprAST(const std::string& callee, Block args)
   {
       this->callee = callee;
       this->args = std::move(args);
   }
};

class NumberExprAST : public ExprAST
{
   int value;

public:
   NumberExprAST(int value)
   {
       this->value = value;
   }
};

static std::unique_ptr<PrototypeAST> ParsePrototype(std::string& content)
{
   std::string FnName = Lexer::identifierstr;
   getNextToken(content);
   std::vector<std::string> ArgNames;
   while (getNextToken(content) == Lexer::tok_identifier)
       ArgNames.push_back(Lexer::identifierstr);
   getNextToken(content);
   return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition(std::string& content)
{
   getNextToken(content); // eat function keyword.
   auto Proto = ParsePrototype(content);
   auto body_block = ParseBlock(content);
   return std::make_unique<FunctionAST>(std::move(Proto), std::move(body_block));
}

static std::unique_ptr<ExprAST> ParsePrimary(std::string& content)
{
   switch (current_token)
   {
   case Lexer::tok_identifier:
       return ParseIdentifierExpr(content);
   case Lexer::tok_number:
       return ParseNumberExpr(content);
   case '(':
       return ParseParenExpr(content);
   case Lexer::tok_return:
       return ParseReturn(content);
   case Lexer::tok_function:
       return ParseDefinition(content);
   }
}

Function* PrototypeAST::codegen()
{
   std::vector<Type*> Ints(args.size(), Type::getInt32Ty(*TheContext));
   FunctionType* FT = FunctionType::get(Type::getInt32Ty(*TheContext), Ints, true);
   Function* F = Function::Create(FT, Function::ExternalLinkage, this->name, TheModule.get());
   return F;
}

Function* FunctionAST::codegen()
{
   Function* TheFunction = this->proto->codegen();
   BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
   Builder->SetInsertPoint(BB);

   for (auto& expr : this->body)
   {
       expr->codegen();
   }
   verifyFunction(*TheFunction);

   return TheFunction;
}

Value* NumberExprAST::codegen()
{
   return ConstantInt::get(Type::getInt32Ty(*TheContext), this->value, true);
}

Value* CallExprAST::codegen()
{
   // Look up the name in the global module table.
   Function* CalleeF = TheModule->getFunction(this->callee);

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
   try
   {
       InitializeAllTargetInfos();
       InitializeAllTargets();
       InitializeAllTargetMCs();
       InitializeAllAsmParsers();
       InitializeAllAsmPrinters();

       TheModule->setTargetTriple(target);
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
   }
   catch (...)
   {
   }
}

void MainLoop(std::string& content)
{
   while (1)
   {
       switch (current_token)
       {
       case Lexer::tok_EOF:
           return;
       case Lexer::tok_function:
           auto F = ParseDefinition(content);
           F->codegen();
           break;
       }
   }
}

int main() {
   std::string content = "def main()\nprint(123)";
   MainLoop(content);
   GenerateObject("x86_64-pc-windows");
}