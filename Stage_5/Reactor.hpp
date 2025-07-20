#pragma once
#include <set>
#include <unordered_map>

// Function that is called when the fd is ready for reading
typedef void* (*reactorFunc)(int fd);

// Internal Reactor class
class Reactor {
public:
    Reactor();
    ~Reactor();

    bool addFd(int fd, reactorFunc func);     // Add fd with its associated function
    bool removeFd(int fd);                    // Remove fd
    void run();                               // Start the select loop
    void stop();                              // Stop the loop
private:
    std::set<int> fds;                        // Set of monitored file descriptors
    std::unordered_map<int, reactorFunc> handlers;  // Map from fd to handler function
    bool running;                             // Is the reactor currently running?
};

// C interface as required by the assignment
extern "C" {
    void* startReactor();                               // Create a new reactor
    int addFdToReactor(void* reactor, int fd, reactorFunc func);
    int removeFdFromReactor(void* reactor, int fd);
    int stopReactor(void* reactor);                     // Only stops – does not delete
    void runReactor(void* reactor);                     // Must be called by the user
}
