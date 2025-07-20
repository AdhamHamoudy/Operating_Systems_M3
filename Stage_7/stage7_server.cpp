// stage7_server.cpp
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <netinet/in.h>
#include <unistd.h>
#include <iomanip>
#include <cstring>

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
std::mutex graph_mutex;
std::unordered_map<int, int> client_graph_input_remaining;
std::mutex client_state_mutex;

// Convex Hull + Area
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

// Helper
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Process single client
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            std::cout << "Client on socket " << client_fd << " disconnected.\n";
            close(client_fd);
            std::lock_guard<std::mutex> lock(client_state_mutex);
            client_graph_input_remaining.erase(client_fd);
            return;
        }

        buffer[bytes_read] = '\0';
        std::istringstream stream(buffer);
        std::string line;

        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            {
                std::lock_guard<std::mutex> lock(client_state_mutex);
                if (client_graph_input_remaining[client_fd] > 0) {
                    std::string l = line;
                    std::replace(l.begin(), l.end(), ',', ' ');
                    std::istringstream ps(l);
                    float x, y;
                    ps >> x >> y;
                    std::lock_guard<std::mutex> g_lock(graph_mutex);
                    global_graph.push_back({x, y});
                    client_graph_input_remaining[client_fd]--;
                    continue;
                }
            }

            if (cmd == "Newgraph") {
                int n;
                iss >> n;
                {
                    std::lock_guard<std::mutex> g_lock(graph_mutex);
                    global_graph.clear();
                }
                {
                    std::lock_guard<std::mutex> lock(client_state_mutex);
                    client_graph_input_remaining[client_fd] = n;
                }
                std::string msg = "Expecting " + std::to_string(n) + " point(s)...\n";
                send(client_fd, msg.c_str(), msg.size(), 0);
            } else if (cmd == "Newpoint") {
                std::string rest;
                std::getline(iss, rest);
                std::replace(rest.begin(), rest.end(), ',', ' ');
                std::istringstream ps(rest);
                float x, y;
                ps >> x >> y;
                std::lock_guard<std::mutex> g_lock(graph_mutex);
                global_graph.push_back({x, y});
            } else if (cmd == "Removepoint") {
                std::string rest;
                std::getline(iss, rest);
                std::replace(rest.begin(), rest.end(), ',', ' ');
                std::istringstream ps(rest);
                float x, y;
                ps >> x >> y;
                Point target = {x, y};
                std::lock_guard<std::mutex> g_lock(graph_mutex);
                auto it = std::find(global_graph.begin(), global_graph.end(), target);
                if (it != global_graph.end()) {
                    global_graph.erase(it);
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
                std::string msg = "Unknown command\n";
                send(client_fd, msg.c_str(), msg.size(), 0);
            }
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 2;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 3;
    }

    std::cout << "Stage 7 server listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "New client connected: " << client_fd << std::endl;
        std::thread(handle_client, client_fd).detach();
    }

    // Close server socket (never reached here in this version)
    close(server_fd);
    return 0;
}
