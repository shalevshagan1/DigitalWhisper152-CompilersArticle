#pragma once

class ExprAST
{
public:
    virtual void codegen() = 0;
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
    void codegen();
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
    void codegen();
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
    void codegen() override;
};

class NumberExprAST : public ExprAST
{
    int value;
   
public:
    NumberExprAST(int value)
    {
        this->value = value;
    }
    void codegen() override;
};