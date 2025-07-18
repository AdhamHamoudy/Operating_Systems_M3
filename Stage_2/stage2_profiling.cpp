#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>

struct Point {
    float x, y;
    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

// Cross product
float cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Shoelace formula
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

// Version A: convex hull using vector
std::vector<Point> convexHullVector(std::vector<Point> P) {
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

// Version B: convex hull using list
std::list<Point> convexHullList(std::list<Point> P) {
    P.sort();
    std::list<Point> H;

    for (auto& p : P) {
        while (H.size() >= 2) {
            auto last = std::prev(H.end());
            auto secondLast = std::prev(last);
            if (cross(*secondLast, *last, p) <= 0)
                H.pop_back();
            else
                break;
        }
        H.push_back(p);
    }

    size_t lowerSize = H.size();

    for (auto it = P.rbegin(); it != P.rend(); ++it) {
        while (H.size() > lowerSize) {
            auto last = std::prev(H.end());
            auto secondLast = std::prev(last);
            if (cross(*secondLast, *last, *it) <= 0)
                H.pop_back();
            else
                break;
        }
        H.push_back(*it);
    }

    H.pop_back(); // remove duplicate
    return H;
}

std::vector<Point> readInput(const std::string& filename) {
    std::ifstream in(filename);
    int n;
    in >> n;
    std::vector<Point> points(n);
    char comma;
    for (int i = 0; i < n; ++i) {
        in >> points[i].x >> comma >> points[i].y;
    }
    return points;
}

int main() {
    std::string inputFile = "input_large.txt";
    std::vector<Point> inputPoints = readInput(inputFile);

    // Version A: vector
    auto startA = std::chrono::high_resolution_clock::now();
    auto hullA = convexHullVector(inputPoints);
    float areaA = polygonArea(hullA);
    auto endA = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeA = endA - startA;

    std::cout << "Vector version:\n";
    std::cout << "  Area: " << areaA << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(6) << timeA.count() << " seconds\n\n";

    // Version B: list
    std::list<Point> listPoints(inputPoints.begin(), inputPoints.end());
    auto startB = std::chrono::high_resolution_clock::now();
    auto hullB = convexHullList(listPoints);

    // Convert list hull to vector for area
    std::vector<Point> hullVecB(hullB.begin(), hullB.end());
    float areaB = polygonArea(hullVecB);
    auto endB = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeB = endB - startB;

    std::cout << "List version:\n";
    std::cout << "  Area: " << areaB << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(6) << timeB.count() << " seconds\n";

    return 0;
}
