#include "codegen.h"
#include <stdexcept>

// The naive stack machine spills every intermediate to memory, so nothing is
// kept live in a caller-saved register across a call — which makes function calls
// correct by construction. Machine-stack slots are 16 bytes so sp stays 16-byte
// aligned at every instruction, satisfying the ABI alignment requirement at each
// call site (even for a call nested inside an expression). See docs/codegen.md.

std::string CodeGen::newLabel(const char* tag) { return ".L" + std::string(tag) + std::to_string(labelId_++); }

void CodeGen::generate(const Program& prog) {
    os_ << "    .text\n";
    for (const auto& fn : prog.functions) genFunction(*fn);
}

void CodeGen::genFunction(const Function& fn) {
    frame_ = fn.frameSize;
    os_ << "    .globl " << fn.name << "\n" << fn.name << ":\n";
    os_ << "    addi sp, sp, -" << frame_ << "\n";
    os_ << "    sw ra, " << (frame_ - 4) << "(sp)\n";
    os_ << "    sw s0, " << (frame_ - 8) << "(sp)\n";
    os_ << "    addi s0, sp, " << frame_ << "\n";
    // Store incoming argument registers a0..a{k-1} into their parameter slots.
    for (size_t i = 0; i < fn.params.size(); ++i)
        os_ << "    sw a" << i << ", " << fn.paramOffsets[i] << "(s0)\n";
    for (const auto& stmt : fn.body) genStatement(*stmt);
    os_ << "    li a0, 0\n";
    emitEpilogue();
}

void CodeGen::emitEpilogue() {
    os_ << "    lw ra, -4(s0)\n    lw t1, -8(s0)\n    mv sp, s0\n    mv s0, t1\n    ret\n";
}

void CodeGen::push() { os_ << "    addi sp, sp, -16\n    sw a0, 0(sp)\n"; }
void CodeGen::pop(const char* reg) { os_ << "    lw " << reg << ", 0(sp)\n    addi sp, sp, 16\n"; }

void CodeGen::genStatement(const Stmt& s) {
    if (const auto* ret = dynamic_cast<const ReturnStmt*>(&s)) { genExpr(*ret->value); emitEpilogue(); return; }
    if (const auto* d = dynamic_cast<const DeclStmt*>(&s)) {
        if (d->init) { genExpr(*d->init); os_ << "    sw a0, " << d->offset << "(s0)\n"; }
        return;
    }
    if (const auto* e = dynamic_cast<const ExprStmt*>(&s)) { genExpr(*e->expr); return; }
    if (dynamic_cast<const EmptyStmt*>(&s)) return;
    if (const auto* b = dynamic_cast<const BlockStmt*>(&s)) { for (const auto& st : b->stmts) genStatement(*st); return; }
    if (const auto* i = dynamic_cast<const IfStmt*>(&s)) {
        std::string lElse = newLabel("else"), lEnd = newLabel("endif");
        genExpr(*i->cond);
        os_ << "    beqz a0, " << (i->else_ ? lElse : lEnd) << "\n";
        genStatement(*i->then_);
        if (i->else_) { os_ << "    j " << lEnd << "\n" << lElse << ":\n"; genStatement(*i->else_); }
        os_ << lEnd << ":\n";
        return;
    }
    if (const auto* w = dynamic_cast<const WhileStmt*>(&s)) {
        std::string lBeg = newLabel("while"), lEnd = newLabel("endwhile");
        os_ << lBeg << ":\n"; genExpr(*w->cond); os_ << "    beqz a0, " << lEnd << "\n";
        genStatement(*w->body); os_ << "    j " << lBeg << "\n" << lEnd << ":\n";
        return;
    }
    if (const auto* f = dynamic_cast<const ForStmt*>(&s)) {
        std::string lBeg = newLabel("for"), lEnd = newLabel("endfor");
        if (f->init) genStatement(*f->init);
        os_ << lBeg << ":\n";
        if (f->cond) { genExpr(*f->cond); os_ << "    beqz a0, " << lEnd << "\n"; }
        genStatement(*f->body);
        if (f->post) genExpr(*f->post);
        os_ << "    j " << lBeg << "\n" << lEnd << ":\n";
        return;
    }
    throw std::runtime_error("codegen: unsupported statement");
}

void CodeGen::genAddr(const Expr& e) {
    if (const auto* v = dynamic_cast<const VarRef*>(&e)) { os_ << "    addi a0, s0, " << v->offset << "\n"; return; }
    if (const auto* u = dynamic_cast<const UnaryExpr*>(&e)) {
        if (u->op == UnOp::Deref) { genExpr(*u->operand); return; }   // address of *p is the value of p
    }
    throw std::runtime_error("codegen: expression is not an lvalue");
}

void CodeGen::genCall(const CallExpr& c) {
    size_t k = c.args.size();
    for (const auto& a : c.args) { genExpr(*a); push(); }   // args pushed arg0..arg{k-1}; arg{k-1} on top
    for (size_t i = 0; i < k; ++i)                          // arg_i sits at sp + 16*(k-1-i)
        os_ << "    lw a" << i << ", " << (16 * (k - 1 - i)) << "(sp)\n";
    if (k) os_ << "    addi sp, sp, " << (16 * k) << "\n";
    os_ << "    call " << c.callee << "\n";                 // result in a0
}

void CodeGen::genExpr(const Expr& e) {
    if (const auto* lit = dynamic_cast<const IntLiteral*>(&e)) { os_ << "    li a0, " << lit->value << "\n"; return; }
    if (const auto* v = dynamic_cast<const VarRef*>(&e)) {
        if (v->type.k == Type::Array) os_ << "    addi a0, s0, " << v->offset << "\n";   // array decays to base address
        else                          os_ << "    lw a0, " << v->offset << "(s0)\n";
        return;
    }
    if (const auto* a = dynamic_cast<const Assign*>(&e)) {
        genExpr(*a->value); push();
        genAddr(*a->target); os_ << "    mv a1, a0\n"; pop("a0");
        os_ << "    sw a0, 0(a1)\n";     // a0 keeps the stored value (assignment result)
        return;
    }
    if (const auto* c = dynamic_cast<const CallExpr*>(&e)) { genCall(*c); return; }
    if (const auto* un = dynamic_cast<const UnaryExpr*>(&e)) {
        switch (un->op) {
            case UnOp::Neg:    genExpr(*un->operand); os_ << "    neg a0, a0\n";  break;
            case UnOp::BitNot: genExpr(*un->operand); os_ << "    not a0, a0\n";  break;
            case UnOp::LogNot: genExpr(*un->operand); os_ << "    seqz a0, a0\n"; break;
            case UnOp::AddrOf: genAddr(*un->operand); break;
            case UnOp::Deref:  genExpr(*un->operand); os_ << "    lw a0, 0(a0)\n"; break;
        }
        return;
    }
    if (const auto* lg = dynamic_cast<const LogicalExpr*>(&e)) {
        if (lg->op == LogOp::And) {
            std::string f = newLabel("andF"), end = newLabel("andE");
            genExpr(*lg->lhs); os_ << "    beqz a0, " << f << "\n";
            genExpr(*lg->rhs); os_ << "    beqz a0, " << f << "\n";
            os_ << "    li a0, 1\n    j " << end << "\n" << f << ":\n    li a0, 0\n" << end << ":\n";
        } else {
            std::string t = newLabel("orT"), end = newLabel("orE");
            genExpr(*lg->lhs); os_ << "    bnez a0, " << t << "\n";
            genExpr(*lg->rhs); os_ << "    bnez a0, " << t << "\n";
            os_ << "    li a0, 0\n    j " << end << "\n" << t << ":\n    li a0, 1\n" << end << ":\n";
        }
        return;
    }
    if (const auto* bin = dynamic_cast<const BinaryExpr*>(&e)) {
        genExpr(*bin->lhs); push();
        genExpr(*bin->rhs); os_ << "    mv a1, a0\n"; pop("a0");   // a0=lhs, a1=rhs
        bool lp = bin->lhs->type.isPointerLike();
        bool rp = bin->rhs->type.isPointerLike();
        switch (bin->op) {
            case BinOp::Add:
                if (lp && !rp)      os_ << "    slli a1, a1, 2\n    add a0, a0, a1\n";   // ptr + int
                else if (!lp && rp) os_ << "    slli a0, a0, 2\n    add a0, a0, a1\n";   // int + ptr
                else                os_ << "    add a0, a0, a1\n";
                break;
            case BinOp::Sub:
                if (lp && !rp)      os_ << "    slli a1, a1, 2\n    sub a0, a0, a1\n";   // ptr - int
                else                os_ << "    sub a0, a0, a1\n";
                break;
            case BinOp::Mul: os_ << "    mul a0, a0, a1\n"; break;
            case BinOp::Div: os_ << "    div a0, a0, a1\n"; break;
            case BinOp::Mod: os_ << "    rem a0, a0, a1\n"; break;
            case BinOp::Eq:  os_ << "    sub a0, a0, a1\n    seqz a0, a0\n"; break;
            case BinOp::Ne:  os_ << "    sub a0, a0, a1\n    snez a0, a0\n"; break;
            case BinOp::Lt:  os_ << "    slt a0, a0, a1\n"; break;
            case BinOp::Gt:  os_ << "    slt a0, a1, a0\n"; break;
            case BinOp::Le:  os_ << "    slt a0, a1, a0\n    xori a0, a0, 1\n"; break;
            case BinOp::Ge:  os_ << "    slt a0, a0, a1\n    xori a0, a0, 1\n"; break;
        }
        return;
    }
    throw std::runtime_error("codegen: unsupported expression");
}
