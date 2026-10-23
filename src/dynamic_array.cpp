#include "dynamic_array.hpp"

#include <iostream>

int main() {
    std::cout << "=== dynamicArray Quick Demo ===\n";

    dynamicArray<int> arr{10, 20, 30, 40, 50};
    arr.push_back(60);

    std::cout << "Elements in dynamicArray<int> (size: " << arr.size()
              << ", capacity: " << arr.capacity() << "):\n";
    for (int x : arr) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    dynamicArray<bool> barr{true, false, true, true, false, true};
    std::cout << "Elements in dynamicArray<bool> (size: " << barr.size()
              << ", bit capacity: " << barr.capacity() << "):\n";
    for (bool b : barr) {
        std::cout << (b ? "1 " : "0 ");
    }
    std::cout << "\n";

    return 0;
}