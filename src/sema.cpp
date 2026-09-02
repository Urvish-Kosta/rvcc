#include "sema.h"
#include <stdexcept>

static int align16(int n) { return (n + 15) & ~15; }
static std::string L(int line) { return line > 0 ? "semantic error (line " + std::to_string(line) + "): " : std::string("semantic error: "); }

int Sema::declare(const std::string& name, Type t, int line) {
    auto& cur = scopes_.back();
    if (cur.count(name)) throw std::runtime_error(L(line) + "redeclaration of '" + name + "' in the same scope");
    int sz = t.sizeBytes();
    int off = -(bytes_ + sz);
    bytes_ += sz;
    cur[name] = Binding{off, t};
    return off;
}

const Sema::Binding& Sema::resolve(const std::string& name, int line) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    throw std::runtime_error(L(line) + "use of undeclared variable '" + name + "'");
}

bool Sema::lvalue(const Expr& e) {
    if (dynamic_cast<const VarRef*>(&e)) return true;
    if (auto* u = dynamic_cast<const UnaryExpr*>(&e)) return u->op == UnOp::Deref;
    return false;
}

void Sema::run(Program& prog) {
    for (auto& fn : prog.functions) {
        if (funcArity_.count(fn->name))
            throw std::runtime_error("semantic error: redefinition of function '" + fn->name + "'");
        if (fn->params.size() > 8)
            throw std::runtime_error(L(fn->params[8].line) + "more than 8 parameters is not supported");
        funcArity_[fn->name] = static_cast<int>(fn->params.size());
    }
    for (auto& fn : prog.functions) function(*fn);
}

void Sema::function(Function& fn) {
    scopes_.clear(); bytes_ = 8;   // reserve saved ra + s0
    pushScope();
    fn.paramOffsets.clear();
    for (auto& p : fn.params) fn.paramOffsets.push_back(declare(p.name, p.type, p.line));
    for (auto& s : fn.body) stmt(*s);
    popScope();
    fn.frameSize = align16(bytes_);
}

void Sema::stmt(Stmt& s) {
    if (auto* d = dynamic_cast<DeclStmt*>(&s)) {
        d->offset = declare(d->name, d->declType, d->line);
        if (d->init) expr(*d->init);
        return;
    }
    if (auto* r = dynamic_cast<ReturnStmt*>(&s)) { expr(*r->value); return; }
    if (auto* e = dynamic_cast<ExprStmt*>(&s))   { expr(*e->expr);  return; }
    if (dynamic_cast<EmptyStmt*>(&s))            { return; }
    if (auto* b = dynamic_cast<BlockStmt*>(&s)) { pushScope(); for (auto& st : b->stmts) stmt(*st); popScope(); return; }
    if (auto* i = dynamic_cast<IfStmt*>(&s)) { expr(*i->cond); stmt(*i->then_); if (i->else_) stmt(*i->else_); return; }
    if (auto* w = dynamic_cast<WhileStmt*>(&s)) { expr(*w->cond); stmt(*w->body); return; }
    if (auto* f = dynamic_cast<ForStmt*>(&s)) {
        pushScope();
        if (f->init) stmt(*f->init);
        if (f->cond) expr(*f->cond);
        if (f->post) expr(*f->post);
        stmt(*f->body);
        popScope();
        return;
    }
    throw std::runtime_error("sema: unsupported statement");
}

void Sema::expr(Expr& e) {
    if (auto* lit = dynamic_cast<IntLiteral*>(&e)) { lit->type = Type::mkInt(); return; }

    if (auto* v = dynamic_cast<VarRef*>(&e)) {
        const Binding& b = resolve(v->name, v->line);
        v->offset = b.offset; v->type = b.type;   // may be Int/Ptr/Array
        return;
    }
    if (auto* a = dynamic_cast<Assign*>(&e)) {
        if (!lvalue(*a->target)) throw std::runtime_error(L(a->line) + "assignment target is not an lvalue");
        expr(*a->target);
        expr(*a->value);
        a->type = Type::mkInt();   // stored/decayed value width is 4 bytes
        return;
    }
    if (auto* u = dynamic_cast<UnaryExpr*>(&e)) {
        expr(*u->operand);
        switch (u->op) {
            case UnOp::AddrOf:
                if (!lvalue(*u->operand)) throw std::runtime_error(L(0) + "cannot take address of non-lvalue");
                u->type = Type::mkPtr(); break;
            case UnOp::Deref:
                if (!u->operand->type.isPointerLike())
                    throw std::runtime_error(L(0) + "cannot dereference a non-pointer");
                u->type = Type::mkInt(); break;
            default:
                u->type = Type::mkInt(); break;    // -, ~, ! yield int
        }
        return;
    }
    if (auto* b = dynamic_cast<BinaryExpr*>(&e)) {
        expr(*b->lhs); expr(*b->rhs);
        bool lp = b->lhs->type.isPointerLike();
        bool rp = b->rhs->type.isPointerLike();
        if (b->op == BinOp::Add) {
            if (lp && rp) throw std::runtime_error(L(0) + "cannot add two pointers");
            b->type = (lp || rp) ? Type::mkPtr() : Type::mkInt();
        } else if (b->op == BinOp::Sub) {
            if (lp && rp) throw std::runtime_error(L(0) + "pointer difference is not supported");
            if (!lp && rp) throw std::runtime_error(L(0) + "cannot subtract a pointer from an integer");
            b->type = lp ? Type::mkPtr() : Type::mkInt();
        } else {
            b->type = Type::mkInt();               // * / % and comparisons
        }
        return;
    }
    if (auto* l = dynamic_cast<LogicalExpr*>(&e)) { expr(*l->lhs); expr(*l->rhs); l->type = Type::mkInt(); return; }

    if (auto* c = dynamic_cast<CallExpr*>(&e)) {
        auto it = funcArity_.find(c->callee);
        if (it == funcArity_.end()) throw std::runtime_error(L(c->line) + "call to undeclared function '" + c->callee + "'");
        if (static_cast<int>(c->args.size()) != it->second)
            throw std::runtime_error(L(c->line) + "function '" + c->callee + "' expects " +
                std::to_string(it->second) + " argument(s) but got " + std::to_string(c->args.size()));
        if (c->args.size() > 8) throw std::runtime_error(L(c->line) + "more than 8 arguments is not supported");
        for (auto& a : c->args) expr(*a);
        c->type = Type::mkInt();
        return;
    }
    throw std::runtime_error("sema: unsupported expression");
}
