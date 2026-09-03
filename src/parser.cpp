#include "parser.h"
#include <stdexcept>

static const char* kindName(TokKind k) {
    switch (k) {
        case TokKind::KwInt: return "'int'"; case TokKind::KwReturn: return "'return'";
        case TokKind::KwIf: return "'if'"; case TokKind::KwElse: return "'else'";
        case TokKind::KwWhile: return "'while'"; case TokKind::KwFor: return "'for'";
        case TokKind::Ident: return "identifier"; case TokKind::Num: return "integer literal";
        case TokKind::LParen: return "'('"; case TokKind::RParen: return "')'";
        case TokKind::LBrace: return "'{'"; case TokKind::RBrace: return "'}'";
        case TokKind::LBracket: return "'['"; case TokKind::RBracket: return "']'";
        case TokKind::Semi: return "';'"; case TokKind::Comma: return "','";
        case TokKind::Plus: return "'+'"; case TokKind::Minus: return "'-'";
        case TokKind::Star: return "'*'"; case TokKind::Slash: return "'/'";
        case TokKind::Percent: return "'%'"; case TokKind::Tilde: return "'~'";
        case TokKind::Bang: return "'!'"; case TokKind::Assign: return "'='";
        case TokKind::EqEq: return "'=='"; case TokKind::BangEq: return "'!='";
        case TokKind::Lt: return "'<'"; case TokKind::Le: return "'<='";
        case TokKind::Gt: return "'>'"; case TokKind::Ge: return "'>='";
        case TokKind::AmpAmp: return "'&&'"; case TokKind::PipePipe: return "'||'";
        case TokKind::Amp: return "'&'"; case TokKind::Eof: return "end of input";
    }
    return "token";
}

const Token& Parser::expect(TokKind k, const char* what) {
    if (!check(k))
        throw std::runtime_error("parse error (line " + std::to_string(peek().line) +
                                 "): expected " + what + " but found " + kindName(peek().kind));
    return advance();
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    while (!check(TokKind::Eof)) prog->functions.push_back(parseFunction());
    return prog;
}

Type Parser::parsePointerStars() {
    // one level of pointer supported: optional single '*'
    if (accept(TokKind::Star)) return Type::mkPtr();
    return Type::mkInt();
}

std::unique_ptr<Function> Parser::parseFunction() {
    expect(TokKind::KwInt, "'int'");
    const Token& name = expect(TokKind::Ident, "function name");
    expect(TokKind::LParen, "'('");
    auto fn = std::make_unique<Function>();
    fn->name = name.text;
    if (!check(TokKind::RParen)) {
        for (;;) {
            expect(TokKind::KwInt, "'int'");
            Type t = parsePointerStars();
            const Token& p = expect(TokKind::Ident, "parameter name");
            fn->params.push_back(Param{p.text, t, p.line});
            if (!accept(TokKind::Comma)) break;
        }
    }
    expect(TokKind::RParen, "')'");
    expect(TokKind::LBrace, "'{'");
    while (!check(TokKind::RBrace)) fn->body.push_back(parseStatement());
    expect(TokKind::RBrace, "'}'");
    return fn;
}

// ---- statements ----
std::unique_ptr<Stmt> Parser::parseDeclaration() {
    expect(TokKind::KwInt, "'int'");
    Type t = parsePointerStars();
    const Token& name = expect(TokKind::Ident, "variable name");
    if (check(TokKind::LBracket)) {                 // int a[N];  (arrays are not pointers here)
        advance();
        const Token& n = expect(TokKind::Num, "array length");
        expect(TokKind::RBracket, "']'");
        if (t.k == Type::Ptr)
            throw std::runtime_error("parse error (line " + std::to_string(name.line) +
                                    "): pointer-to-array is not supported");
        t = Type::mkArray(static_cast<int>(nr.value));
    }
    std::unique_ptr<Expr> init;
    if (accept(TokKind::Assign)) {
        if (t.k == Type::Array)
            throw std::runtime_error("parse error (line " + std::to_string(name.line) +
                                     "): array initialisers are not supported");
        init = parseExpression();
    }
    expect(TokKind::Semi, "';'");
    return std::make_unique<DeclStmt>(name.text, name.line, t, std::move(init));
}

std::unique_ptr<Stmt> Parser::parseBlock() {
    expect(TokKind::LBrace, "'{'");
    auto blk = std::make_unique<BlockStmt>();
    while (!check(TokKind::RBrace)) blk->stmts.push_back(parseStatement());
    expect(TokKind::RBrace, "'}'");
    return blk;
}
std::unique_ptr<Stmt> Parser::parseIf() {
    expect(TokKind::KwIf, "'if'"); expect(TokKind::LParen, "'('");
    auto cond = parseExpression(); expect(TokKind::RParen, "')'");
    auto then_ = parseStatement();
    std::unique_ptr<Stmt> else_;
    if (accept(TokKind::KwElse)) else_ = parseStatement();
    return std::make_unique<IfStmt>(std::move(cond), std::move(then_), std::move(else_));
}
std::unique_ptr<Stmt> Parser::parseWhile() {
    expect(TokKind::KwWhile, "'while'"); expect(TokKind::LParen, "'('");
    auto cond = parseExpression(); expect(TokKind::RParen, "')'");
    return std::make_unique<WhileStmt>(std::move(cond), parseStatement());
}
std::unique_ptr<Stmt> Parser::parseFor() {
    expect(TokKind::KwFor, "'for'"); expect(TokKind::LParen, "'('");
    auto f = std::make_unique<ForStmt>();
    if (check(TokKind::Semi)) advance();
    else if (check(TokKind::KwInt)) f->init = parseDeclaration();
    else { auto e = parseExpression(); expect(TokKind::Semi, "';'"); f->init = std::make_unique<ExprStmt>(std::move(e)); }
    if (!check(TokKind::Semi)) f->cond = parseExpression();
    expect(TokKind::Semi, "';'");
    if (!check(TokKind::RParen)) f->post = parseExpression();
    expect(TokKind::RParen, "')'");
    f->body = parseStatement();
    return f;
}
std::unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokKind::KwInt))    return parseDeclaration();
    if (check(TokKind::LBrace))   return parseBlock();
    if (check(TokKind::KwIf))     return parseIf();
    if (check(TokKind::KwWhile))  return parseWhile();
    if (check(TokKind::KwFor))    return parseFor();
    if (accept(TokKind::Semi))    return std::make_unique<EmptyStmt>();
    if (check(TokKind::KwReturn)) { advance(); auto v = parseExpression(); expect(TokKind::Semi, "';'"); return std::make_unique<ReturnStmt>(std::move(v)); }
    auto e = parseExpression(); expect(TokKind::Semi, "';'"); return std::make_unique<ExprStmt>(std::move(e));
}

// ---- expressions ----
std::unique_ptr<Expr> Parser::parseExpression() { return parseAssignment(); }

static bool isLvalue(const Expr* e) {
    if (dynamic_cast<const VarRef*>(e)) return true;
    if (auto* u = dynamic_cast<const UnaryExpr*>(e)) return u->op == UnOp::Deref;
    return false;
}
std::unique_ptr<Expr> Parser::parseAssignment() {
    auto lhs = parseLogicalOr();
    if (check(TokKind::Assign)) {
        const Token& eq = advance();
        auto rhs = parseAssignment();
        if (isLvalue(lhs.get()))
            return std::make_unique<Assign>(std::move(lhs), std::move(rhs), eq.line);
        throw std::runtime_error("parse error (line " + std::to_string(eq.line) +
                                 "): left-hand side of '=' is not assignable");
    }
    return lhs;
}
std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto n = parseLogicalAnd();
    while (check(TokKind::PipePipe)) { advance(); n = std::make_unique<LogicalExpr>(LogOp::Or, std::move(n), parseLogicalAnd()); }
    return n;
}
std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto n = parseEquality();
    while (check(TokKind::AmpAmp)) { advance(); n = std::make_unique<LogicalExpr>(LogOp::And, std::move(n), parseEquality()); }
    return n;
}
std::unique_ptr<Expr> Parser::parseEquality() {
    auto n = parseRelational();
    for (;;) {
        if (accept(TokKind::EqEq))  n = std::make_unique<BinaryExpr>(BinOp::Eq, std::move(n), parseRelational());
        else if (accept(TokKind::BangEq)) n = std::make_unique<BinaryExpr>(BinOp::Ne, std::move(n), parseRelational());
        else return n;
    }
}
std::unique_ptr<Expr> Parser::parseRelational() {
    auto n = parseAdditive();
    for (;;) {
        if (accept(TokKind::Lt)) n = std::make_unique<BinaryExpr>(BinOp::Lt, std::move(n), parseAdditive());
        else if (accept(TokKind::Le)) n = std::make_unique<BinaryExpr>(BinOp::Le, std::move(n), parseAdditive());
        else if (accept(TokKind::Gt)) n = std::make_unique<BinaryExpr>(BinOp::Gt, std::move(n), parseAdditive());
        else if (accept(TokKind::Ge)) n = std::make_unique<BinaryExpr>(BinOp::Ge, std::move(n), parseAdditive());
        else return n;
    }
}
std::unique_ptr<Expr> Parser::parseAdditive() {
    auto n = parseMultiplicative();
    for (;;) {
        if (accept(TokKind::Plus))  n = std::make_unique<BinaryExpr>(BinOp::Add, std::move(n), parseMultiplicative());
        else if (accept(TokKind::Minus)) n = std::make_unique<BinaryExpr>(BinOp::Sub, std::move(n), parseMultiplicative());
        else return n;
    }
}
std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto n = parseUnary();
    for (;;) {
        if (accept(TokKind::Star)) n = std::make_unique<BinaryExpr>(BinOp::Mul, std::move(n), parseUnary());
        else if (accept(TokKind::Slash)) n = std::make_unique<BinaryExpr>(BinOp::Div, std::move(n), parseUnary());
        else if (accept(TokKind::Percent)) n = std::make_unique<BinaryExpr>(BinOp::Mod, std::move(n), parseUnary());
        else return n;
    }
}
std::unique_ptr<Expr> Parser::parseUnary() {
    if (accept(TokKind::Minus)) return std::make_unique<UnaryExpr>(UnOp::Neg,    parseUnary());
    if (accept(TokKind::Tilde)) return std::make_unique<UnaryExpr>(UnOp::BitNot, parseUnary());
    if (accept(TokKind::Bang))  return std::make_unique<UnaryExpr>(UnOp::LogNot, parseUnary());
    if (accept(TokKind::Amp))   return std::make_unique<UnaryExpr>(UnOp::AddrOf, parseUnary());
    if (accept(TokKind::Star))  return std::make_unique<UnaryExpr>(UnOp::Deref,  parseUnary());
    return parsePostfix();
}
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto n = parsePrimary();
    while (check(TokKind::LBracket)) {                 // a[i]  ==  *(a + i)
        advance();
        auto idx = parseExpression();
        expect(TokKind::RBracket, "']'");
        auto add = std::make_unique<BinaryExpr>(BinOp::Add, std::move(n), std::move(idx));
        n = std::make_unique<UnaryExpr>(UnOp::Deref, std::move(add));
    }
    return n;
}
std::unique_ptr<Expr> Parser::parsePrimary() {
    if (accept(TokKind::LParen)) { auto e = parseExpression(); expect(TokKind::RParen, "')'"); return e; }
    if (check(TokKind::Ident)) {
        const Token& id = advance();
        if (check(TokKind::LParen)) {                  // call
            advance();
            auto call = std::make_unique<CallExpr>(id.text, id.line);
            if (!check(TokKind::RParen)) {
                for (;;) { call->args.push_back(parseExpression()); if (!accept(TokKind::Comma)) break; }
            }
            expect(TokKind::RParen, "')'");
            return call;
        }
        return std::make_unique<VarRef>(id.text, id.line);
    }
    const Token& t = expect(TokKind::Num, "integer literal, variable, or '('");
    return std::make_unique<IntLiteral>(t.value);
}
