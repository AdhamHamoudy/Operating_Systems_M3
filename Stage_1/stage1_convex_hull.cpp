#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>

struct Point {
    float x, y;
    bool operator<(const Point& p) const {
        return (x < p.x || (x == p.x && y < p.y));
    }
};

float cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

std::vector<Point> convexHull(std::vector<Point>& P) {
    int n = P.size(), k = 0;
    if (n <= 1) return P;

    std::sort(P.begin(), P.end());
    std::vector<Point> H(2 * n);

    // Lower hull
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }

    // Upper hull
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

int main() {
    int n;
    std::cin >> n;
    std::vector<Point> points(n);

    char comma;
    for (int i = 0; i < n; ++i) {
        std::cin >> points[i].x >> comma >> points[i].y;
    }

    std::vector<Point> hull = convexHull(points);
    float area = polygonArea(hull);

    std::cout << std::fixed << std::setprecision(6) << area << std::endl;
    return 0;
}
