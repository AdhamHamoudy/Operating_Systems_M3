// stage9_server.cpp
#include "../Stage_8/Reactor.hpp"
#include <iostream>
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

// Shared state
std::vector<Point> global_graph;
std::unordered_map<int, int> client_graph_input_remaining;
std::mutex graph_mutex;
std::mutex client_state_mutex;

// Utilities
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

// Client handler function
void* handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string input_buffer;

    send(client_fd, "Welcome to the convex hull server!\n", 35, 0);

    while (true) {
        ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            close(client_fd);
            std::lock_guard<std::mutex> lock(client_state_mutex);
            client_graph_input_remaining.erase(client_fd);
            return nullptr;
        }

        buffer[bytes] = '\0';
        input_buffer += buffer;

        size_t pos;
        while ((pos = input_buffer.find('\n')) != std::string::npos) {
            std::string line = trim(input_buffer.substr(0, pos));
            input_buffer.erase(0, pos + 1);

            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            if (cmd == "Newgraph") {
                int n;
                if (iss >> n) {
                    {
                        std::lock_guard<std::mutex> g_lock(graph_mutex);
                        global_graph.clear();
                    }
                    {
                        std::lock_guard<std::mutex> lock(client_state_mutex);
                        client_graph_input_remaining[client_fd] = n;
                    }
                    std::ostringstream msg;
                    msg << "Expecting " << n << " point(s)...\n";
                    send(client_fd, msg.str().c_str(), msg.str().size(), 0);
                }
            } else if (cmd == "Newpoint") {
                float x, y;
                if (iss >> x >> y) {
                    std::lock_guard<std::mutex> g_lock(graph_mutex);
                    global_graph.push_back({x, y});
                }
            } else if (cmd == "Removepoint") {
                float x, y;
                if (iss >> x >> y) {
                    Point target = {x, y};
                    std::lock_guard<std::mutex> g_lock(graph_mutex);
                    auto it = std::find(global_graph.begin(), global_graph.end(), target);
                    if (it != global_graph.end()) {
                        global_graph.erase(it);
                    }
                }
            } else if (cmd == "CH") {
                std::vector<Point> local_copy;
                {
                    std::lock_guard<std::mutex> g_lock(graph_mutex);
                    local_copy = global_graph;
                }
                std::vector<Point> hull = convexHull(local_copy);
                float area = polygonArea(hull);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << area << "\n";
                std::string out = oss.str();
                send(client_fd, out.c_str(), out.size(), 0);
            } else {
                // Try parse as point input if in Newgraph state
                float x, y;
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream ps(line);
                if (ps >> x >> y) {
                    bool accepted = false;
                    {
                        std::lock_guard<std::mutex> lock(client_state_mutex);
                        if (client_graph_input_remaining[client_fd] > 0) {
                            std::lock_guard<std::mutex> g_lock(graph_mutex);
                            global_graph.push_back({x, y});
                            client_graph_input_remaining[client_fd]--;
                            accepted = true;
                        }
                    }
                    if (accepted) {
                        std::ostringstream oss;
                        oss << "Added point: (" << x << "," << y << ")\n";
                        send(client_fd, oss.str().c_str(), oss.str().size(), 0);
                    } else {
                        send(client_fd, "Unexpected point. Use Newgraph first.\n", 38, 0);
                    }
                } else {
                    send(client_fd, "Invalid command.\n", 18, 0);
                }
            }
        }
    }

    return nullptr;
}

// Main function
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 2;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 3;
    }

    std::cout << "[Stage 9] Server listening on port " << PORT << "...\n";

    pthread_t tid = startProactor(server_fd, handle_client);
    pthread_join(tid, nullptr);

    close(server_fd);
    return 0;
}
