// sema.h — semantic analysis / name+type resolution.
// M4: collects function signatures (name -> arity) for call checking and forward
//     references; binds parameters as the first locals; enforces <=8 params/args.
// M5: a minimal type system (int, pointer-to-int, array-of-int). Annotates every
//     Expr with its Type so codegen can scale pointer arithmetic and choose
//     load/address forms. Arrays are sized in the frame (N*4 bytes).
#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>

class Sema {
public:
    void run(Program& prog);
private:
    struct Binding { int offset; Type type; };
    std::vector<std::unordered_map<std::string, Binding>> scopes_;
    std::unordered_map<std::string, int> funcArity_;   // name -> param count
    int bytes_ = 0;

    void pushScope() { scopes_.emplace_back(); }
    void popScope()  { scopes_.pop_back(); }
    int  declare(const std::string& name, Type t, int line);
    const Binding& resolve(const std::string& name, int line) const;

    void function(Function& fn);
    void stmt(Stmt& s);
    void expr(Expr& e);                       // sets e.type
    static bool lvalue(const Expr& e);
};
