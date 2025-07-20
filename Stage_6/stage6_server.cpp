
// stage6_server.cpp
#include "Reactor.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>

#define PORT 9034
#define BUFFER_SIZE 1024

// ========= Shared Graph Data =========
struct Point {
    float x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

std::vector<Point> global_graph;
std::unordered_map<int, int> client_points_remaining;

float cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

std::vector<Point> convexHull(std::vector<Point> P) {
    int n = P.size(), k = 0;
    if (n <= 1) return P;
    std::sort(P.begin(), P.end());
    std::vector<Point> H(2 * n);
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }
    for (int i = n - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }
    H.resize(k - 1);
    return H;
}

float polygonArea(const std::vector<Point>& poly) {
    float area = 0.0f;
    int n = poly.size();
    for (int i = 0; i < n; ++i) {
        const Point& p1 = poly[i];
        const Point& p2 = poly[(i + 1) % n];
        area += (p1.x * p2.y) - (p2.x * p1.y);
    }
    return std::abs(area) / 2.0f;
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" 	
");
    size_t end = s.find_last_not_of(" 	
");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

void* reactor = nullptr;

void* handle_client(int fd) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytesRead <= 0) {
        std::cout << "[Server] Client disconnected (fd=" << fd << ")
";
        close(fd);
        removeFdFromReactor(reactor, fd);
        return nullptr;
    }

    std::string input = trim(buffer);
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    if (command == "Newgraph") {
        int count;
        iss >> count;
        client_points_remaining[fd] = count;
        global_graph.clear();
        std::string msg = "[Server] Ready for " + std::to_string(count) + " points\n";
        send(fd, msg.c_str(), msg.size(), 0);
    } else if (client_points_remaining.count(fd) && client_points_remaining[fd] > 0) {
        float x, y;
        char comma;
        std::istringstream pointStream(input);
        pointStream >> x >> comma >> y;
        global_graph.push_back({x, y});
        client_points_remaining[fd]--;
        std::string msg = "[Server] Point received\n";
        send(fd, msg.c_str(), msg.size(), 0);
    } else if (command == "CH") {
        std::vector<Point> hull = convexHull(global_graph);
        float area = polygonArea(hull);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << area << "\n";
        std::string result = oss.str();
        send(fd, result.c_str(), result.size(), 0);
    } else {
        std::string msg = "[Server] Unknown command\n";
        send(fd, msg.c_str(), msg.size(), 0);
    }

    return nullptr;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    std::cout << "[Server] Listening on port " << PORT << "\n";
    reactor = startReactor();

    addFdToReactor(reactor, server_fd, [](int fd) -> void* {
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd >= 0) {
            std::cout << "[Server] New client connected (fd=" << client_fd << ")\n";
            addFdToReactor(reactor, client_fd, handle_client);
        }
        return nullptr;
    });

    runReactor(reactor);
    close(server_fd);
    return 0;
}
