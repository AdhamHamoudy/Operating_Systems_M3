# Makefile for Stage 7 Server

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
TARGET = stage7_server
SRCS = stage7_server.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) $(OBJS)

run: all
	./$(TARGET)

killport:
	@echo "Killing process using port 9034..."
	-@fuser -k 9034/tcp || echo "No process found using port 9034."

.PHONY: all clean run killport
