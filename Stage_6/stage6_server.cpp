#include "Reactor.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iomanip>

#define PORT 9034
#define BUFFER_SIZE 1024

// ========== Data Structures ==========

struct Point {
    float x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

// ========== Global Graph and State ==========

std::vector<Point> global_graph;
std::unordered_map<int, int> client_expected_points;
std::unordered_map<int, std::string> client_buffers;
void* reactor = nullptr; // global pointer to reactor

// ========== Geometry ==========

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
    float area = 0.0;
    int n = poly.size();
    for (int i = 0; i < n; ++i) {
        const Point& p1 = poly[i];
        const Point& p2 = poly[(i + 1) % n];
        area += (p1.x * p2.y) - (p2.x * p1.y);
    }
    return std::abs(area) / 2.0f;
}

// ========== Helpers ==========

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// ========== Client Handler ==========

void* handle_client(int fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(fd);
        client_expected_points.erase(fd);
        client_buffers.erase(fd);
        return nullptr;
    }

    buffer[bytes_read] = '\0';
    client_buffers[fd] += buffer;

    std::string& full = client_buffers[fd];
    size_t pos = 0;
    std::string line;

    while ((pos = full.find('\n')) != std::string::npos) {
        line = full.substr(0, pos);
        full.erase(0, pos + 1);

        line = trim(line);
        if (line.empty()) continue;

        if (client_expected_points[fd] > 0) {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ps(line);
            float x, y;
            ps >> x >> y;
            global_graph.push_back({x, y});
            client_expected_points[fd]--;
            continue;
        }

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "Newgraph") {
            int n;
            iss >> n;
            global_graph.clear();
            client_expected_points[fd] = n;
            std::string msg = "Expecting " + std::to_string(n) + " point(s)...\n";
            send(fd, msg.c_str(), msg.size(), 0);
        } else if (cmd == "Newpoint") {
            std::string rest;
            std::getline(iss, rest);
            std::replace(rest.begin(), rest.end(), ',', ' ');
            std::istringstream ps(rest);
            float x, y;
            ps >> x >> y;
            global_graph.push_back({x, y});
        } else if (cmd == "Removepoint") {
            std::string rest;
            std::getline(iss, rest);
            std::replace(rest.begin(), rest.end(), ',', ' ');
            std::istringstream ps(rest);
            float x, y;
            ps >> x >> y;
            Point target = {x, y};
            auto it = std::find(global_graph.begin(), global_graph.end(), target);
            if (it != global_graph.end()) {
                global_graph.erase(it);
            }
        } else if (cmd == "CH") {
            std::vector<Point> hull = convexHull(global_graph);
            float area = polygonArea(hull);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << area << "\n";
            std::string out = oss.str();
            send(fd, out.c_str(), out.size(), 0);
        } else {
            std::string msg = "Unknown command\n";
            send(fd, msg.c_str(), msg.size(), 0);
        }
    }

    return nullptr;
}

// ========== Accept Handler (no lambda capture) ==========

void* accept_handler(int fd) {
    sockaddr_in client_addr{};
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(fd, (sockaddr*)&client_addr, &addrlen);
    if (client_fd >= 0) {
        std::cout << "New connection: " << client_fd << "\n";
        addFdToReactor(reactor, client_fd, handle_client);
    }
    return nullptr;
}

// ========== Main ==========

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 2;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 3;
    }

    std::cout << "Server (Stage 6) running on port " << PORT << "...\n";

    reactor = startReactor();
    addFdToReactor(reactor, server_fd, accept_handler);
    runReactor(reactor);
    stopReactor(reactor);
    close(server_fd);
    return 0;
}
