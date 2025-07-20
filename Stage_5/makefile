# === Compiler and Flags ===
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# === Source Files ===
REACTOR_SRC = Reactor.cpp
REACTOR_HDR = Reactor.hpp
TEST_SRC = test_reactor.cpp
TARGET = test_reactor

# === Build Target ===
all: $(TARGET)

$(TARGET): $(REACTOR_SRC) $(REACTOR_HDR) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(REACTOR_SRC) $(TEST_SRC)

# === Run ===
run: $(TARGET)
	./$(TARGET)

# === Clean ===
clean:
	rm -f $(TARGET)
