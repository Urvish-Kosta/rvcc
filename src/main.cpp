// main.cpp — rvcc driver.  Usage: rvcc <input.c> [-o <output.s>]
// Reads C source, runs lexer -> parser -> codegen, writes RV32IM assembly.
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open input file: " + path);
    std::ostringstream ss; ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    std::string input, output;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) { output = argv[++i]; }
        else if (a == "-o") { std::cerr << "rvcc: -o requires an argument\n"; return 2; }
        else if (input.empty()) { input = a; }
        else { std::cerr << "rvcc: unexpected argument '" << a << "'\n"; return 2; }
    }
    if (input.empty()) {
        std::cerr << "usage: rvcc <input.c> [-o <output.s>]\n";
        return 2;
    }

    try {
        std::string src = readFile(input);
        Lexer lexer(src);
        auto toks = lexer.tokenize();
        Parser parser(std::move(toks));
        auto program = parser.parseProgram();
        Sema().run(*program);

        if (output.empty()) {
            CodeGen cg(std::cout);
            cg.generate(*program);
        } else {
            std::ofstream out(output);
            if (!out) throw std::runtime_error("cannot open output file: " + output);
            CodeGen cg(out);
            cg.generate(*program);
        }
    } catch (const std::exception& ex) {
        std::cerr << "rvcc: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
