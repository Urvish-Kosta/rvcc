// token.h — token kinds for the rvcc lexer.
#pragma once
#include <string>

enum class TokKind {
    // keywords
    KwInt, KwReturn, KwIf, KwElse, KwWhile, KwFor,
    Ident, Num,
    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket, Semi, Comma,
    // arithmetic
    Plus, Minus, Star, Slash, Percent, Tilde, Bang,
    Assign,        // =
    // comparison (M3)
    EqEq, BangEq, Lt, Le, Gt, Ge,     // == != < <= > >=
    // logical (M3)
    AmpAmp, PipePipe,                 // && ||
    Amp,                              // & (address-of)
    Eof
};

struct Token {
    TokKind kind;
    std::string text;
    long value = 0;
    int line = 0;
};
