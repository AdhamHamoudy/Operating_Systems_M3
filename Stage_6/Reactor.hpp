#pragma once
#include <set>
#include <unordered_map>

// Function pointer type used by the reactor
typedef void* (*reactorFunc)(int fd);

class Reactor {
public:
    Reactor();
    ~Reactor();

    bool addFd(int fd, reactorFunc func);
    bool removeFd(int fd);
    void run();
    void stop();

private:
    std::set<int> fds;
    std::unordered_map<int, reactorFunc> handlers;
    bool running;
};

// C-style interface
extern "C" {
    void* startReactor();
    int addFdToReactor(void* reactor, int fd, reactorFunc func);
    int removeFdFromReactor(void* reactor, int fd);
    int stopReactor(void* reactor);
    void runReactor(void* reactor);
}
