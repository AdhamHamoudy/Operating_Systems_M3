// stage8_server.cpp
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
#include <mutex>

#define PORT 9034
#define BUFFER_SIZE 1024

struct Point {
    float x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

// Shared data
std::vector<Point> global_graph;
std::unordered_map<int, int> client_points_remaining;
std::mutex graph_mutex;

// Convex Hull
float cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

std::vector<Point> convexHull(std::vector<Point> P) {
    int n = P.size(), k = 0;
    if (n <= 1) return P;
    std::sort(P.begin(), P.end());
    std::vector<Point> H(2 * n);
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }
    for (int i = n - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
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
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Proactor client thread handler
void* handleClient(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string input_buffer;

    send(client_fd, "Welcome to the convex hull server!\n", 35, 0);

    while (true) {
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            close(client_fd);
            return nullptr;
        }

        buffer[bytes_received] = '\0';
        input_buffer += buffer;

        size_t pos;
        while ((pos = input_buffer.find('\n')) != std::string::npos) {
            std::string line = trim(input_buffer.substr(0, pos));
            input_buffer.erase(0, pos + 1);

            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "Newgraph") {
                int n;
                if (iss >> n) {
                    std::lock_guard<std::mutex> lock(graph_mutex);
                    global_graph.clear();
                    client_points_remaining[client_fd] = n;

                    std::ostringstream msg;
                    msg << "Expecting " << n << " point(s)...\n";
                    send(client_fd, msg.str().c_str(), msg.str().size(), 0);
                }
            } else if (command == "CH") {
                std::lock_guard<std::mutex> lock(graph_mutex);
                if (client_points_remaining[client_fd] == 0 && !global_graph.empty()) {
                    std::vector<Point> hull = convexHull(global_graph);
                    float area = polygonArea(hull);
                    std::ostringstream oss;
                    oss << "Convex Hull Area: " << area << "\n";
                    send(client_fd, oss.str().c_str(), oss.str().size(), 0);
                } else {
                    std::string err = "Not enough points or points still pending. Use Newgraph.\n";
                    send(client_fd, err.c_str(), err.size(), 0);
                }
            } else {
                // Handle point input
                std::istringstream point_line(line);
                float x, y;
                if (point_line >> x >> y) {
                    std::lock_guard<std::mutex> lock(graph_mutex);
                    if (client_points_remaining[client_fd] > 0) {
                        global_graph.push_back({x, y});
                        client_points_remaining[client_fd]--;

                        std::ostringstream oss;
                        oss << "Added point: (" << x << "," << y << ")\n";
                        send(client_fd, oss.str().c_str(), oss.str().size(), 0);

                        if (client_points_remaining[client_fd] == 0) {
                            send(client_fd, "All points received. You may now run CH.\n", 42, 0);
                        }
                    } else {
                        send(client_fd, "Unexpected point. Use Newgraph first.\n", 38, 0);
                    }
                } else {
                    send(client_fd, "Invalid command or point.\n", 26, 0);
                }
            }
        }
    }

    return nullptr;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "[Server] Listening on port " << PORT << "...\n";
    pthread_t proactor_thread = startProactor(server_fd, handleClient);

    pthread_join(proactor_thread, nullptr);
    close(server_fd);
    return 0;
}
