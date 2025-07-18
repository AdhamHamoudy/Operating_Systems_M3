# Makefile for Stage 4 - Multi-user TCP Convex Hull Server

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
TARGET = stage4_server
SRC = stage4_server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
