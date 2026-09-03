#include "lexer.h"
#include <cctype>
#include <stdexcept>

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while (!eof()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) { advance(); continue; }

        // Comments.
        if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
            while (!eof() && peek() != '\n') advance();
            continue;
        }
        if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
            advance(); advance();
            while (!eof() && !(peek() == '*' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/')) advance();
            if (!eof()) { advance(); advance(); }
            continue;
        }

        int startLine = line_;

        // Numbers.
        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) num += advance();
            Token t; t.kind = TokKind::Num; t.text = num; t.value = std::stol(num); t.line = startLine;
            out.push_back(t);
            continue;
        }

        // Identifiers / keywords.
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string id;
            while (!eof() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) id += advance();
            Token t; t.text = id; t.line = startLine;
            if      (id == "int")    t.kind = TokKind::KwInt;
            else if (id == "return") t.kind = TokKind::KwReturn;
            else if (id == "if")     t.kind = TokKind::KwIf;
            else if (id == "else")   t.kind = TokKind::KwElse;
            else if (id == "while")  t.kind = TokKind::KwWhile;
            else if (id == "for")    t.kind = TokKind::KwFor;
            else                     t.kind = TokKind::Ident;
            out.push_back(t);
            continue;
        }

        // Operators and punctuation. Two-character operators are matched first.
        char c1 = c;
        char c2 = (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0';
        Token t; t.line = startLine;
        auto emit2 = [&](TokKind k, const char* txt){ advance(); advance(); t.kind = k; t.text = txt; };
        auto emit1 = [&](TokKind k){ advance(); t.kind = k; t.text = std::string(1, c1); };

        if      (c1 == '=' && c2 == '=') emit2(TokKind::EqEq,     "==");
        else if (c1 == '!' && c2 == '=') emit2(TokKind::BangEq,   "!=");
        else if (c1 == '<' && c2 == '=') emit2(TokKind::Le,       "<=");
        else if (c1 == '>' && c2 == '=') emit2(TokKind::Ge,       ">=");
        else if (c1 == '&' && c2 == '&') emit2(TokKind::AmpAmp,   "&&");
        else if (c1 == '|' && c2 == '|') emit2(TokKind::PipePipe, "||");
        else if (c1 == '=') emit1(TokKind::Assign);
        else if (c1 == '!') emit1(TokKind::Bang);
        else if (c1 == '<') emit1(TokKind::Lt);
        else if (c1 == '>') emit1(TokKind::Gt);
        else if (c1 == '(') emit1(TokKind::LParen);
        else if (c1 == ')') emit1(TokKind::RParen);
        else if (c1 == '{') emit1(TokKind::LBrace);
        else if (c1 == '}') emit1(TokKind::RBrace);
        else if (c1 == ';') emit1(TokKind::Semi);
        else if (c1 == ',') emit1(TokKind::Comma);
        else if (c1 == '+') emit1(TokKind::Plus);
        else if (c1 == '-') emit1(TokKind::Minus);
        else if (c1 == '*') emit1(TokKind::Star);
        else if (c1 == '/') emit1(TokKind::Slash);
        else if (c1 == '%') emit1(TokKind::Percent);
        else if (c1 == '~') emit1(TokKind::Tilde);
        else if (c1 == '&') emit1(TokKind::Amp);
        else if (c1 == '[') emit1(TokKind::LBracket);
        else if (c1 == ']') emit1(TokKind::RBracket);
        else {
            throw std::runtime_error("lex error (line " + std::to_string(startLine) +
                                    "): unexpected character '" + std::string(1, c1) + "'");
        }
        out.push_back(t);
    }
    Token eofTok; eofTok.kind = TokKind::Eof; eofTok.line = line_;
    out.push_back(eofTok);
    return out;
}
