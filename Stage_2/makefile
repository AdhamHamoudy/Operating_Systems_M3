# Makefile for Stage 2: Convex Hull Profiling

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

GEN_SRC = generate_input.cpp
GEN_BIN = generate_input

PROF_SRC = stage2_profiling.cpp
PROF_BIN = stage2_profiling

INPUT = input_large.txt

all: $(GEN_BIN) $(PROF_BIN)

# Compile the input generator
$(GEN_BIN): $(GEN_SRC)
	$(CXX) $(CXXFLAGS) -o $(GEN_BIN) $(GEN_SRC)

# Compile the profiler
$(PROF_BIN): $(PROF_SRC)
	$(CXX) $(CXXFLAGS) -o $(PROF_BIN) $(PROF_SRC)

# Run generator to create input
gen: $(GEN_BIN)
	./$(GEN_BIN)

# Run profiler (assumes input already generated)
test: $(PROF_BIN) $(INPUT)
	./$(PROF_BIN)

run: gen test

clean:
	rm -f $(GEN_BIN) $(PROF_BIN) $(INPUT)
