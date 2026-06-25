#include "dynamic_array.hpp"

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <string>

template <typename T>
void pushBackAndPrint(dynamicArray<T>& vec, T x) {
    vec.push_back(x);
    std::cout << "size: " << vec.size() << "\n";
    std::cout << "capacity: " << vec.capacity() << "\n";
    std::cout << "current contents: ";

    for (std::size_t i{0}; i < vec.size(); ++i) {
        std::cout << vec[i] << " ";
    }

    std::cout << "\n\n";
}

template <typename T>
void print(dynamicArray<T>& vec) {
    std::cout << "size: " << vec.size() << "\n";
    std::cout << "capacity: " << vec.capacity() << "\n";
    std::cout << "current contents: ";

    for (std::size_t i{0}; i < vec.size(); ++i) {
        std::cout << vec[i] << " ";
    }

    std::cout << "\n\n";
}

void testReallocation() {
    dynamicArray<int> myVec{};

    pushBackAndPrint(myVec, 1);
    pushBackAndPrint(myVec, 2);
    pushBackAndPrint(myVec, 3);
    pushBackAndPrint(myVec, 4);
    pushBackAndPrint(myVec, 5);
    pushBackAndPrint(myVec, 6);
    pushBackAndPrint(myVec, 7);
    pushBackAndPrint(myVec, 8);
    pushBackAndPrint(myVec, 9);
    pushBackAndPrint(myVec, 10);
}

void testCopyConstructor() {
    dynamicArray<int> a{};
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.push_back(4);
    dynamicArray<int> b(a);
    b.push_back(5);
    print(b);
}

void testCopyAssignmentConstructor() {
    dynamicArray<int> a{};
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.push_back(4);

    dynamicArray<int> b{};
    b.push_back(5);
    b.push_back(6);
    b.push_back(7);

    b = a;
    print(b);
}

void testRangeBasedLoop() {
    dynamicArray<std::string> msg;
    msg.push_back("one");
    msg.push_back("chance");
    msg.push_back("bang!");

    for (const auto& word : msg) {
        std::cout << word << " ";
    }
}

void testDefaultInit() {
    dynamicArray<std::string> msg{"once", "chance", "...", "bang!"};

    for (const auto& word : msg) {
        std::cout << word << " ";
    }
}

void testCustomIterator() {  // need to implement custom iterator although will work with raw
                             // pointers
    dynamicArray<std::string> myVec{"once", "chance", "...", "bang!"};

    for (auto it = myVec.begin(); it != myVec.end(); ++it) {
        std::cout << *it;
    }
}

void testChangingIndex() {
    dynamicArray<std::string> myVec{"once", "chance"};
    myVec[0] = "I'm";
    myVec[1] = "Back";
    print(myVec);
}

int main() {
    // testReallocation();
    // testCopyConstructor();
    // testCopyAssignmentConstructor();
    // testRangeBasedLoop();
    // testDefaultInit();
    // testCustomIterator();
    testChangingIndex();
    return 0;
}