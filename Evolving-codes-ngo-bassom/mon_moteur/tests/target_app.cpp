#include <iostream>

int compute(int x) {
    return x + x; 
}

int main() {
    std::cout << "Starting target program..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        int res = compute(i);
        if (i % 200 == 0) std::cout << "Iter " << i << " - Result: " << res << std::endl;
    }
    std::cout << "Target finished." << std::endl;
    return 0;
}