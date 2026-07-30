#include "dynamic_array.hpp"

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <string>

template <typename T>
void basicPrint(dynamicArray<T>& vec) {
    if (vec.size() == 0) {
        std::cout << "null\n";
        return;
    }
    for (auto i : vec) {
        std::cout << i << " ";
    }

    std::cout << "\n\n";
}

void testCopyConstructor() {
    dynamicArray<int> a{1, 2, 3};
    dynamicArray<int> b(a);
    basicPrint(b);
}

void testCopyAssignment() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    dynamicArray<int> b;
    b = a;
    basicPrint(b);
}

void testCustomIterator() {
    dynamicArray<std::string> myVec{"once", "chance", "...", "bang!"};
    for (auto it = myVec.begin(); it != myVec.end(); ++it) {
        std::cout << *it;
    }
}

void testCustomIteratorConstVec() {
    const dynamicArray<std::string> myVec{"once", "chance", "...", "bang!"};
    // shouldnt be able to deref indexes and edit them
    for (auto it = myVec.begin(); it != myVec.end(); ++it) {
        std::cout << *it;
    }
}

void testDefaultInit() {
    dynamicArray<std::string> msg{"once", "chance", "...", "bang!"};

    for (const auto& word : msg) {
        std::cout << word << " ";
    }
}

void testMoveConstructor() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    dynamicArray<int> b{std::move(a)};
    std::cout << "a: ";
    basicPrint(a);
    std::cout << "b: ";
    basicPrint(b);
}

void testMoveAssignment() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    dynamicArray<int> b{6, 7, 8};
    b = std::move(a);
    std::cout << "a: ";
    basicPrint(a);
    std::cout << "b: ";
    basicPrint(b);
}

void testPushBack() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    std::cout << "a: ";
    basicPrint(a);
    a.push_back(6);
    a.push_back(7);
    std::cout << "after pushback: ";
    basicPrint(a);
}

void testInitPushback() {
    dynamicArray<int> a;
    std::cout << "a: ";
    basicPrint(a);
    a.push_back(6);
    a.push_back(7);
    std::cout << "after pushback: ";
    basicPrint(a);
}

void testMovePushBack() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    std::cout << "a: ";
    basicPrint(a);
    a.push_back(6);
    a.push_back(7);
    std::cout << "after pushback: ";
    basicPrint(a);
}

void printIndex() {
    dynamicArray<int> a{1, 2, 3, 4, 5};
    std::cout << a[4];
}

void testBoolInit() {
    dynamicArray<bool> a{true, true, false};
    // for (auto i : a) { //needs proper iterator
    //     std::cout << static_cast<int>(i) << " ";
    // }
}

int main() {
    // testCopyConstructor();
    // testCopyAssignment();
    // testMoveConstructor();
    // testMoveAssignment();
    // testPushBack();
    // testInitPushback();
    // testMovePushBack();
    // printIndex();
    testBoolInit();
    return 0;
}