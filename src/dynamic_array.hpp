#pragma once
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <string>

template <typename T>
class dynamicArray {
   private:
    std::size_t m_size;
    std::size_t m_capacity;
    T* m_data;

   public:
    dynamicArray() : m_size(0), m_capacity(0), m_data(nullptr) {};  // default constructor

    dynamicArray(
        const std::initializer_list<T>& list)  // overloaded constructor w/ default initialiser
        : m_size(static_cast<std::size_t>(list.size())),
          m_capacity(static_cast<std::size_t>(list.size())),
          m_data(new T[static_cast<std::size_t>(list.size())]) {
        std::size_t i{0};
        for (auto val : list) {
            m_data[i++] = val;
        }
    }

    ~dynamicArray() { delete[] m_data; }  // destructor

    dynamicArray(const dynamicArray<T>& other)  // copy constructor
        : m_size(other.m_size), m_capacity(other.m_capacity), m_data(new T[other.m_capacity]) {
        for (std::size_t i{0}; i < other.m_size; ++i) {
            m_data[i] = other.m_data[i];
        }
    }

    dynamicArray& operator=(const dynamicArray<T>& other) {  // copy assignment operator
        if (this == &other) {
            return *this;
        }

        delete[] m_data;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        m_data = new T[m_capacity];
        for (std::size_t i{0}; i < m_capacity; ++i) {
            m_data[i] = other.m_data[i];
        }

        return *this;
    }

    void push_back(const T& x) {
        if (m_data == nullptr) {
            m_capacity += 1;
            m_data = new T[m_capacity];
        }

        if (m_size == m_capacity) {
            m_capacity *= 2;
            T* tempPointer{new T[m_capacity]};

            for (std::size_t i{0}; i < m_size; ++i) {  // copy contents of old array to new one

                tempPointer[i] = m_data[i];
            }

            delete[] m_data;
            m_data = tempPointer;
            tempPointer = nullptr;
        }

        m_data[m_size] = x;
        m_size++;
    }

    std::size_t size() const { return m_size; }
    std::size_t capacity() const { return m_capacity; }
    T get(std::size_t index) const { return m_data[index]; }
    T* begin() const { return &m_data[0]; };
    T* end() const { return &m_data[0] + m_size; };
};