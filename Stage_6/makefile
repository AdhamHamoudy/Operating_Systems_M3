CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: stage6_server

stage6_server: stage6_server.cpp Reactor.cpp Reactor.hpp
	$(CXX) $(CXXFLAGS) -o stage6_server stage6_server.cpp Reactor.cpp

clean:
	rm -f stage6_server
