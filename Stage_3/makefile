# Makefile for Stage 3 - Interactive Convex Hull

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRC = stage3_interactive.cpp
BIN = stage3_interactive

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
