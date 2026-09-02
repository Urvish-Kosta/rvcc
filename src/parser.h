#pragma once
#include "ast.h"
#include "token.h"
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}
    std::unique_ptr<Program> parseProgram();
private:
    std::vector<Token> toks_;
    size_t pos_ = 0;
    const Token& peek() const { return toks_[pos_]; }
    const Token& peek2() const { return toks_[pos_ + 1 < toks_.size() ? pos_ + 1 : pos_]; }
    const Token& advance() { return toks_[pos_++]; }
    bool check(TokKind k) const { return peek().kind == k; }
    bool accept(TokKind k) { if (check(k)) { advance(); return true; } return false; }
    const Token& expect(TokKind k, const char* what);

    std::unique_ptr<Function> parseFunction();
    Type parsePointerStars();                      // consume optional leading '*' (one level)

    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseDeclaration();
    std::unique_ptr<Stmt> parseBlock();
    std::unique_ptr<Stmt> parseIf();
    std::unique_ptr<Stmt> parseWhile();
    std::unique_ptr<Stmt> parseFor();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
};
