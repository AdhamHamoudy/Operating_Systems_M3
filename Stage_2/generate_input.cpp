#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <iomanip>

int main() {
    std::ofstream out("input_large.txt");
    int n = 10000;
    out << n << "\n";

    std::srand(std::time(nullptr));
    for (int i = 0; i < n; ++i) {
        float x = static_cast<float>(rand() % 10000) / 10.0;
        float y = static_cast<float>(rand() % 10000) / 10.0;
        out << std::fixed << std::setprecision(2) << x << "," << y << "\n";
    }

    out.close();
    std::cout << "input_large.txt generated with " << n << " points.\n";
    return 0;
}
