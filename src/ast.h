// ast.h — abstract syntax tree.
// M4: multiple functions, parameters, CallExpr; Assign now targets a general
//     lvalue (a variable or, from M5, a dereference).
// M5: pointer/array types; UnaryExpr gains AddrOf/Deref; array indexing desugars
//     to *(base + index) in the parser. Sema annotates each Expr with a Type.
#pragma once
#include <memory>
#include <string>
#include <vector>

// ---- Types (minimal: int, pointer-to-int, array-of-int) ----
struct Type {
    enum K { Int, Ptr, Array } k = Int;
    int arrayLen = 0;                         // valid when k == Array
    static Type mkInt()            { return Type{Int, 0}; }
    static Type mkPtr()            { return Type{Ptr, 0}; }
    static Type mkArray(int n)     { return Type{Array, n}; }
    bool isPointerLike() const     { return k == Ptr || k == Array; }
    int  sizeBytes() const         { return k == Array ? 4 * arrayLen : 4; }
};

// ---- Expressions ----
struct Expr { virtual ~Expr() = default; Type type; };   // type filled by sema

struct IntLiteral : Expr { long value; explicit IntLiteral(long v){ value=v; type=Type::mkInt(); } };

enum class UnOp { Neg, BitNot, LogNot, AddrOf, Deref };
struct UnaryExpr : Expr {
    UnOp op; std::unique_ptr<Expr> operand;
    UnaryExpr(UnOp o,std::unique_ptr<Expr> e):op(o),operand(std::move(e)){}
};

enum class BinOp { Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Le, Gt, Ge };
struct BinaryExpr : Expr {
    BinOp op; std::unique_ptr<Expr> lhs, rhs;
    BinaryExpr(BinOp o,std::unique_ptr<Expr> l,std::unique_ptr<Expr> r)
        :op(o),lhs(std::move(l)),rhs(std::move(r)){}
};

enum class LogOp { And, Or };
struct LogicalExpr : Expr {
    LogOp op; std::unique_ptr<Expr> lhs, rhs;
    LogicalExpr(LogOp o,std::unique_ptr<Expr> l,std::unique_ptr<Expr> r)
        :op(o),lhs(std::move(l)),rhs(std::move(r)){}
};

struct VarRef : Expr {
    std::string name; int line; int offset = 0;
    VarRef(std::string n,int ln):name(std::move(n)),line(ln){}
};

struct Assign : Expr {                        // target = value  (target is an lvalue)
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
    int line;
    Assign(std::unique_ptr<Expr> t,std::unique_ptr<Expr> v,int ln)
        :target(std::move(t)),value(std::move(v)),line(ln){}
};

struct CallExpr : Expr {
    std::string callee; int line;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(std::string c,int ln):callee(std::move(c)),line(ln){}
};

// ---- Statements ----
struct Stmt { virtual ~Stmt() = default; };
struct ReturnStmt : Stmt { std::unique_ptr<Expr> value; explicit ReturnStmt(std::unique_ptr<Expr> v):value(std::move(v)){} };
struct DeclStmt : Stmt {
    std::string name; int line; int offset = 0; Type declType;
    std::unique_ptr<Expr> init;
    DeclStmt(std::string n,int ln,Type t,std::unique_ptr<Expr> i)
        :name(std::move(n)),line(ln),declType(t),init(std::move(i)){}
};
struct ExprStmt : Stmt { std::unique_ptr<Expr> expr; explicit ExprStmt(std::unique_ptr<Expr> e):expr(std::move(e)){} };
struct EmptyStmt : Stmt {};
struct BlockStmt : Stmt { std::vector<std::unique_ptr<Stmt>> stmts; };
struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond; std::unique_ptr<Stmt> then_, else_;
    IfStmt(std::unique_ptr<Expr> c,std::unique_ptr<Stmt> t,std::unique_ptr<Stmt> e):cond(std::move(c)),then_(std::move(t)),else_(std::move(e)){}
};
struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond; std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> c,std::unique_ptr<Stmt> b):cond(std::move(c)),body(std::move(b)){}
};
struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init; std::unique_ptr<Expr> cond; std::unique_ptr<Expr> post; std::unique_ptr<Stmt> body;
};

// ---- Top level ----
struct Param { std::string name; Type type; int line; };
struct Function {
    std::string name;
    std::vector<Param> params;
    std::vector<int> paramOffsets;             // filled by sema, parallel to params
    std::vector<std::unique_ptr<Stmt>> body;
    int frameSize = 0;
};
struct Program { std::vector<std::unique_ptr<Function>> functions; };
