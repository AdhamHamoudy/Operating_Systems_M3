# Makefile for compiling stage9_server with Proactor support

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

# Source files
SRCS = stage9_server.cpp ../Stage_8/Reactor.cpp

# Output executable
TARGET = stage9_server

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

killport:
	@echo "Killing process using port 9034..."
	-@fuser -k 9034/tcp || echo "No process found using port 9034."

.PHONY: all clean run killport
