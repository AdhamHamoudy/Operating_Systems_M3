#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <csignal>

// ========== Proactor Definitions ==========
typedef void* (*proactorFunc)(int sockfd);

struct ClientThreadArgs {
    int client_fd;
    proactorFunc handler;
};

void* client_thread_start(void* arg) {
    ClientThreadArgs* args = static_cast<ClientThreadArgs*>(arg);
    int fd = args->client_fd;
    proactorFunc handler = args->handler;
    delete args;
    return handler(fd);
}

struct ProactorArgs {
    int sockfd;
    proactorFunc handler;
};

void* proactorLoop(void* arg) {
    ProactorArgs* args = static_cast<ProactorArgs*>(arg);
    int sockfd = args->sockfd;
    proactorFunc handler = args->handler;
    delete args;

    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(sockfd, (sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }

        pthread_t client_thread;

        // create a wrapper args struct for client handler
        ClientThreadArgs* ct_args = new ClientThreadArgs{client_fd, handler};

        // start the client thread using the wrapper
        pthread_create(&client_thread, nullptr, client_thread_start, ct_args);
        pthread_detach(client_thread);
    }

    return nullptr;
}

pthread_t startProactor(int sockfd, proactorFunc handler) {
    pthread_t tid;
    ProactorArgs* args = new ProactorArgs{sockfd, handler};
    pthread_create(&tid, nullptr, proactorLoop, args);
    return tid;
}

int stopProactor(pthread_t tid) {
    return pthread_cancel(tid);
}
