#include <vector>
#include <string>
#include "types.h"


class Instruction {
    std::string NAME;
    std::vector<std::string> params;


    std::string generate_asm() {
        std::string asm_code = this->NAME;

        for (std::string param : params)
        {
            asm_code += " " + param;
        }

        return asm_code;
    }
};

class MOV : Instruction {

};

class PUSH : Instruction {

};

class POP : Instruction {
    
};

class INVOKE : Instruction {

};

class PROTOTYPE : Instruction {

};



class generator {
    std::string processor = ""; // default is left to the assembler
    std::string model = ""; // default is left to the assembler
    std::string headers = ""; 
    std::string codeseg;
    std::string dataseg;

    std::vector<Instruction> Instructions;

    void assemble_print(std::vector<std::unique_ptr<ExprAST>> args) 
    {
        // add 3 function prototypes to headers
        
        // add INVOKE calls
    }

    void assemble_proc(std::string name)
    {
        codeseg += name + " PROC";
        for (Instruction i : Instructions)
        {

        }
        codeseg += "END " + name;
    }
};

generator gen;
void FunctionAST::codegen() {

    for (auto expr : this->body)
    {
        expr->codegen();
    }

}

void CallExprAST::codegen()
{
    if (this->callee == "print")
    {
        gen::assemble_print(this->args); // add print to the assembly, it's an external function
    }


}
// print(123)