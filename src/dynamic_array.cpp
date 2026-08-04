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
    dynamicArray<bool> a{true, true, true, true, true, false};
    for (std::size_t i{0}; i < a.size(); ++i) {
        std::cout << a[i] << " ";
    }
}

void testBoolIndexAssign() {
    dynamicArray<bool> a{true};
    std::cout << "before: " << a[0];
    a[0] = false;
    std::cout << " after: " << a[0];
}

void testBoolAssignOtherInd() {
    dynamicArray<bool> a{true, false};

    std::cout << "before: " << a[0] << " " << a[1] << "\n";
    a[1] = a[0];
    std::cout << "after: " << a[0] << " " << a[1];
}

void testBoolCopyConstructor() {
    dynamicArray<bool> a{false, false};
    std::cout << "a: " << a[0] << " " << a[1] << "\n";
    dynamicArray<bool> b{a};
    std::cout << "b: " << b[0] << " " << b[1] << "\n";
}

void testBoolCopyAssignment() {
    dynamicArray<bool> a{false, false};
    std::cout << "a: " << a[0] << " " << a[1] << "\n";
    dynamicArray<bool> b{true, true};
    std::cout << "b: " << b[0] << " " << b[1] << "\n";
    a = b;
    std::cout << "a after assignemnt: " << a[0] << " " << a[1] << "\n";
}

void testBoolMoveConstructor() {
    dynamicArray<bool> a{false, false};
    std::cout << "a: " << a[0] << " " << a[1] << "\n";
    dynamicArray<bool> b{std::move(a)};
    std::cout << "b: " << b[0] << " " << b[1] << "\n";
}

void testBoolMoveAssignment() {
    dynamicArray<bool> a{false, false};
    std::cout << "a: " << a[0] << " " << a[1] << "\n";
    dynamicArray<bool> b{true, true};
    std::cout << "b: " << b[0] << " " << b[1] << "\n";
    b = std::move(a);
    std::cout << "b after move: " << b[0] << " " << b[1] << "\n";
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

    // testBoolInit();
    // testBoolIndexAssign();
    // testBoolAssignOtherInd();
    // testBoolCopyConstructor();
    // testBoolCopyAssignment();
    // testBoolMoveConstructor();
    testBoolMoveAssignment();
    return 0;
}