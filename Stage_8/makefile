CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

TARGET = stage8_server
SRCS = stage8_server.cpp Reactor.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o

killport:
	@echo "Killing process using port 9034..."
	-@fuser -k 9034/tcp || echo "No process found using port 9034."

.PHONY: all clean run killport