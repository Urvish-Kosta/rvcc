# Simple build with no dependencies beyond a C++17 compiler.
# `cmake` is also supported (see CMakeLists.txt) but not required.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc
SRCS     := src/lexer.cpp src/parser.cpp src/sema.cpp src/codegen.cpp src/main.cpp
BIN      := build/rvcc

$(BIN): $(SRCS) $(wildcard src/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(BIN)

.PHONY: test clean
test: $(BIN)
	python3 tests/run_tests.py

clean:
	rm -rf build
