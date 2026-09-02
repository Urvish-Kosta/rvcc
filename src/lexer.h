// lexer.h — hand-written tokenizer (no generator, keeps it self-contained/readable).
#pragma once
#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string src) : src_(std::move(src)) {}
    std::vector<Token> tokenize();   // throws std::runtime_error on bad input
private:
    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
    char peek() const { return pos_ < src_.size() ? src_[pos_] : '\0'; }
    char advance() { char c = src_[pos_++]; if (c == '\n') ++line_; return c; }
    bool eof() const { return pos_ >= src_.size(); }
};
