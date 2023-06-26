#pragma once
#include <memory>
class Instruction {
public:
    std::string NAME;
    std::vector<std::string> params;

    virtual std::string assemble() = 0;
};

class MOV : Instruction {
public:
    MOV(std::string dest, std::string source);
    std::string assemble() override;
};

class PUSHAD : Instruction {
public:
    PUSHAD();
    std::string assemble() override;
};


class POPAD : Instruction {
public:
    POPAD();
    std::string assemble() override;
};

class INVOKE : Instruction {
public:
    std::string expression;
    INVOKE(std::string expression, std::vector<std::string> arguments);
    std::string assemble() override;
};

class PROTOTYPE : Instruction {
public:
    std::string label;
    PROTOTYPE(std::string label);
    std::string assemble() override;
};

class Generator {
public:
    std::string processor = ""; // default is left to the assembler
    std::string model = ""; // default is left to the assembler
    std::string headers = ""; 
    std::string codeseg = ".code\n";
    std::string dataseg = ".data\n";
    std::vector<Instruction> Instructions;
    void assemble_print(std::vector<std::unique_ptr<ExprAST>> args);
    void assemble_proc(std::string name, std::unique_ptr<ExprAST> expr);
    void assemble_number(int value);
    void output_all_asm();
};

Generator gen;
// std::map<std::string, std::size_t> symbol_table;