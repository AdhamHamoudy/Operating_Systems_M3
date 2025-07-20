# Makefile for compiling stage10_server with Proactor (Stage 8) support

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

# Source files
SRCS = stage10_server.cpp ../Stage_8/Reactor.cpp

# Output executable
TARGET = stage10_server

# Build target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Run the server
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Clean up compiled files
.PHONY: clean
clean:
	rm -f $(TARGET)

# Kill any process using port 9034
.PHONY: killport
killport:
	@echo "Killing process using port 9034..."
	-@fuser -k 9034/tcp || echo "No process found using port 9034."
