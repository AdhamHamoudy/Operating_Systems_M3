# Makefile for Stage 1 - Convex Hull

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
TARGET = stage1
SRC = stage1_convex_hull.cpp
TEST_INPUT = input.txt

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET) $(TEST_INPUT)
	./$(TARGET) < $(TEST_INPUT)

clean:
	rm -f $(TARGET)
