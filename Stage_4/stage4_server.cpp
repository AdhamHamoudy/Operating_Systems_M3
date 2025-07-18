#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <iomanip>
#include <unordered_map>

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

// Global graph shared by all clients
std::vector<Point> global_graph;

// Track per-client input state (for Newgraph)
std::unordered_map<int, int> client_graph_input_remaining;

// Convex Hull helpers
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

// Trim helper
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Process line from client
void handle_client_input(int client_fd, const std::string& raw_line) {
    std::string line = trim(raw_line);
    if (line.empty()) return;

    // Check if client is in the middle of Newgraph point input
    if (client_graph_input_remaining[client_fd] > 0) {
        std::string l = line;
        std::replace(l.begin(), l.end(), ',', ' ');
        std::istringstream iss(l);
        float x, y;
        iss >> x >> y;
        global_graph.push_back({x, y});
        client_graph_input_remaining[client_fd]--;
        return;
    }

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "Newgraph") {
        int n;
        iss >> n;
        global_graph.clear();
        client_graph_input_remaining[client_fd] = n;
        std::string msg = "Expecting " + std::to_string(n) + " point(s)...\n";
        send(client_fd, msg.c_str(), msg.size(), 0);
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
        send(client_fd, out.c_str(), out.size(), 0);
    } else {
        std::string msg = "Unknown command\n";
        send(client_fd, msg.c_str(), msg.size(), 0);
    }
}

// ========================= MAIN =========================

int main() {
    int listener_fd, new_fd;
    struct sockaddr_in server_addr{}, client_addr{};
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    fd_set master_fds, read_fds;
    int fdmax;

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(2);
    }

    if (listen(listener_fd, 10) < 0) {
        perror("listen");
        exit(3);
    }

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    FD_SET(listener_fd, &master_fds);
    fdmax = listener_fd;

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        read_fds = master_fds;
        if (select(fdmax + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener_fd) {
                    new_fd = accept(listener_fd, (struct sockaddr*)&client_addr, &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(new_fd, &master_fds);
                        if (new_fd > fdmax) fdmax = new_fd;
                        std::cout << "New connection on socket " << new_fd << std::endl;
                    }
                } else {
                    int bytes_read = recv(i, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_read <= 0) {
                        if (bytes_read == 0) {
                            std::cout << "Socket " << i << " disconnected\n";
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_graph_input_remaining.erase(i);
                    } else {
                        buffer[bytes_read] = '\0';
                        std::istringstream iss(buffer);
                        std::string line;
                        while (std::getline(iss, line)) {
                            handle_client_input(i, line);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
