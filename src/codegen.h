#pragma once
#include "ast.h"
#include <ostream>
#include <string>

class CodeGen {
public:
    explicit CodeGen(std::ostream& os) : os_(os) {}
    void generate(const Program& prog);   // requires Sema to have run
private:
    std::ostream& os_;
    int frame_ = 0;
    int labelId_ = 0;
    std::string newLabel(const char* tag);

    void genFunction(const Function& fn);
    void genStatement(const Stmt& s);
    void genExpr(const Expr& e);    // value in a0
    void genAddr(const Expr& e);    // address of an lvalue in a0
    void genCall(const CallExpr& c);
    void emitEpilogue();
    void push();                     // push a0 (16-byte slot, keeps sp 16-aligned)
    void pop(const char* reg);
};
