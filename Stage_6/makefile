CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I../Stage_5

all: stage6_server

stage6_server: stage6_server.cpp ../Stage_5/Reactor.cpp
	$(CXX) $(CXXFLAGS) -o stage6_server stage6_server.cpp ../Stage_5/Reactor.cpp

clean:
	rm -f stage6_server
