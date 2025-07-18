#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <iomanip>

struct Point {
    float x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

// Cross product
float cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Convex Hull (Monotone Chain)
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

// Area using Shoelace Formula
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

int main() {
    std::vector<Point> points;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "Newgraph") {
            int n;
            iss >> n;
            points.clear();
            for (int i = 0; i < n; ++i) {
                std::getline(std::cin, line);
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream pstream(line);
                float x, y;
                pstream >> x >> y;
                points.push_back({x, y});
            }
        } else if (command == "Newpoint") {
            float x, y;
            std::string rest;
            std::getline(iss, rest);
            std::replace(rest.begin(), rest.end(), ',', ' ');
            std::istringstream pstream(rest);
            pstream >> x >> y;
            points.push_back({x, y});
        } else if (command == "Removepoint") {
            float x, y;
            std::string rest;
            std::getline(iss, rest);
            std::replace(rest.begin(), rest.end(), ',', ' ');
            std::istringstream pstream(rest);
            pstream >> x >> y;
            Point target = {x, y};
            auto it = std::find(points.begin(), points.end(), target);
            if (it != points.end()) {
                points.erase(it);
            }
        } else if (command == "CH") {
            std::vector<Point> hull = convexHull(points);
            float area = polygonArea(hull);
            std::cout << std::fixed << std::setprecision(6) << area << std::endl;
        }
    }

    return 0;
}
