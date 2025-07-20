#pragma once
#include <pthread.h>

// ======== Typedefs ========
typedef void (*reactorFunc)(int fd);           // Reactor callback function
typedef void* (*proactorFunc)(int sockfd);     // Proactor callback function

// ======== Reactor Interface ========
void* startReactor();
int addFdToReactor(void* reactor, int fd, reactorFunc func);
int removeFdFromReactor(void* reactor, int fd);
int stopReactor(void* reactor);

// ======== Proactor Interface ========
pthread_t startProactor(int sockfd, proactorFunc threadFunc);
int stopProactor(pthread_t tid);
