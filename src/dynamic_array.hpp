#pragma once
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

template <typename T>
class dynamicArray {
   private:
    std::size_t m_size;
    std::size_t m_capacity;
    std::unique_ptr<T[]> m_data;

   public:
    class Iterator {
       private:
        T* m_ptr;

       public:
        Iterator(T* ptr) : m_ptr(ptr) {};
        T& operator*() { return *m_ptr; };
        Iterator& operator++() {
            ++m_ptr;
            return *this;
        }
        bool operator!=(const Iterator& other) const { return m_ptr != other.m_ptr; }
    };

    class ConstIterator {
       private:
        const T* m_ptr;

       public:
        ConstIterator(const T* ptr) : m_ptr(ptr) {};
        const T& operator*() { return *m_ptr; };
        ConstIterator& operator++() {
            ++m_ptr;
            return *this;
        }
        bool operator!=(const ConstIterator& other) const { return m_ptr != other.m_ptr; }
    };

    dynamicArray() : m_size(0), m_capacity(0), m_data(nullptr) {};  // default constructor

    dynamicArray(
        const std::initializer_list<T>& list)  // overloaded constructor w/ default initialiser
        : m_size(list.size()), m_capacity(list.size()), m_data(std::make_unique<T[]>(list.size())) {
        std::size_t i{0};
        for (auto val : list) {
            m_data[i++] = val;
        }
    }

    dynamicArray(const dynamicArray<T>& other)  // copy constructor
        : m_size(other.m_size),
          m_capacity(other.m_capacity),
          m_data(std::make_unique<T[]>(other.m_capacity)) {
        std::copy(other.m_data.get(), other.m_data.get() + other.m_size, m_data.get());
    }

    dynamicArray& operator=(const dynamicArray<T>& other) {  // copy assignment operator
        if (this == &other) {
            return *this;
        }
        auto temp{std::make_unique<T[]>(other.m_capacity)};
        std::copy(other.m_data.get(), other.m_data.get() + other.m_size, temp.get());
        m_data = std::move(temp);
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        return *this;
    }

    dynamicArray(dynamicArray<T>&& other) noexcept  // move constructor
        : m_size(other.m_size), m_capacity(other.m_capacity), m_data(std::move(other.m_data)) {
        other.m_capacity = 0;
        other.m_size = 0;
    }

    dynamicArray& operator=(dynamicArray<T>&& other) noexcept {  // move assignment
        if (this == &other) {
            return *this;
        }
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        m_data = std::move(other.m_data);

        other.m_capacity = 0;
        other.m_size = 0;
        return *this;
    }

    void push_back(const T& x) {
        if (m_size == m_capacity) {
            m_capacity = (m_capacity == 0) ? 1 : m_capacity * 2;
            auto temp{std::make_unique<T[]>(m_capacity)};
            std::copy(m_data.get(), m_data.get() + m_size, temp.get());
            m_data = std::move(temp);
        }
        m_data[m_size] = x;
        ++m_size;
    }

    std::size_t size() const { return m_size; }
    std::size_t capacity() const { return m_capacity; }
    T& operator[](std::size_t index) { return m_data[index]; }
    const T& operator[](std::size_t index) const { return m_data[index]; }
    Iterator begin() { return m_data.get(); }
    Iterator end() { return m_data.get() + m_size; }
    ConstIterator begin() const { return m_data.get(); }
    ConstIterator end() const { return m_data.get() + m_size; }
};