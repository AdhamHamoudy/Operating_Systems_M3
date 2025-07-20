#include "Reactor.hpp"
#include <iostream>
#include <unistd.h>  // pipe, read, write
#include <cstring>
#include <thread>
#include <chrono>

void* myCallback(int fd) {
    char buffer[128];
    int n = read(fd, buffer, sizeof(buffer)-1);
    if (n > 0) {
        buffer[n] = '\0';
        std::cout << "[Reactor] Received: " << buffer;
    }
    return nullptr;
}

int main() {
    int fds[2]; // pipe: fds[0] for reading, fds[1] for writing
    pipe(fds);

    void* reactor = startReactor();
    addFdToReactor(reactor, fds[0], myCallback);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        write(fds[1], "Hello!\n", 7);
    });

    std::thread runner([&]() {
        runReactor(reactor);  // this blocks
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    stopReactor(reactor);

    runner.join();
    writer.join();

    return 0;
}
