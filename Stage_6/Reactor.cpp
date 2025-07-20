#include "Reactor.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <iostream>

Reactor::Reactor() : running(false) {}

Reactor::~Reactor() {
    stop();
}

bool Reactor::addFd(int fd, reactorFunc func) {
    if (fds.count(fd)) return false;
    fds.insert(fd);
    handlers[fd] = func;
    return true;
}

bool Reactor::removeFd(int fd) {
    if (!fds.count(fd)) return false;
    fds.erase(fd);
    handlers.erase(fd);
    return true;
}

void Reactor::run() {
    running = true;
    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = 0;

        for (int fd : fds) {
            FD_SET(fd, &readfds);
            if (fd > maxfd) maxfd = fd;
        }

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (activity < 0) continue;

        for (int fd : fds) {
            if (FD_ISSET(fd, &readfds)) {
                if (handlers.count(fd)) {
                    handlers[fd](fd);
                }
            }
        }
    }
}

void Reactor::stop() {
    running = false;
}

// ==== C interface ====

extern "C" {

void* startReactor() {
    return static_cast<void*>(new Reactor());
}

int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    return static_cast<Reactor*>(reactor)->addFd(fd, func) ? 0 : -1;
}

int removeFdFromReactor(void* reactor, int fd) {
    return static_cast<Reactor*>(reactor)->removeFd(fd) ? 0 : -1;
}

int stopReactor(void* reactor) {
    static_cast<Reactor*>(reactor)->stop();
    return 0;
}

void runReactor(void* reactor) {
    static_cast<Reactor*>(reactor)->run();
}
}
