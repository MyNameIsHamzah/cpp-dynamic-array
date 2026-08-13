#pragma once
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

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

    void push_back(const T& x) {  // takes l value refs
        if (m_size == m_capacity) {
            m_capacity = (m_capacity == 0) ? 1 : m_capacity * 2;
            auto temp{std::make_unique<T[]>(m_capacity)};
            std::copy(m_data.get(), m_data.get() + m_size, temp.get());
            m_data = std::move(temp);
        }
        m_data[m_size] = x;
        ++m_size;
    }

    void push_back(
        T&& x) {  // takes r value refs - more commonly used but takes advantage of move semantics.
        if (m_size == m_capacity) {
            m_capacity = (m_capacity == 0) ? 1 : m_capacity * 2;
            auto temp{std::make_unique<T[]>(m_capacity)};
            std::copy(m_data.get(), m_data.get() + m_size, temp.get());
            m_data = std::move(temp);
        }
        m_data[m_size] = std::move(x);
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

template <>
class dynamicArray<bool> {
   private:
    std::size_t m_count;
    std::size_t m_byteCapacity;
    std::unique_ptr<std::uint8_t[]> m_data;
    static std::size_t calculateBucket(std::size_t globalIdx) { return globalIdx / 8; }
    static std::uint8_t calculateByteIndex(std::size_t globalIdx) { return 1 << (globalIdx % 8); }

   public:
    template <bool isConst>
    class ProxyImpl {
       private:
        using m_uint8_t = std::conditional_t<isConst, const std::uint8_t, std::uint8_t>;

        m_uint8_t* m_bytePtr;
        std::uint8_t m_bitMask;

       public:
        ProxyImpl(m_uint8_t* bytePtr, std::uint8_t bitMask)
            : m_bytePtr{bytePtr}, m_bitMask{bitMask} {}

        ProxyImpl& operator=(bool val)
            requires(!isConst)
        {
            if (val) {
                *m_bytePtr |= m_bitMask;
            } else {
                *m_bytePtr &= ~m_bitMask;
            }
            return *this;
        }
        ProxyImpl& operator=(const ProxyImpl& other)
            requires(!isConst)
        {
            return *this = static_cast<bool>(other);
        }
        operator bool() const noexcept { return ((*m_bytePtr & m_bitMask) != 0); }
    };
    using Proxy = ProxyImpl<false>;
    using ConstProxy = ProxyImpl<true>;

    template <bool isConst>
    class IteratorImpl {
       private:
        using m_uint8_t = std::conditional_t<isConst, const std::uint8_t, std::uint8_t>;
        using m_proxy = std::conditional_t<isConst, ConstProxy, Proxy>;

        m_uint8_t* m_bytePtr;
        std::uint8_t m_bitMask;

       public:
        using iterator_category = std::forward_iterator_tag;  // for now? bidirectional support? NO
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using reference = m_proxy;
        using pointer = void;

        IteratorImpl(m_uint8_t* bytePtr, std::uint8_t bitMask)
            : m_bytePtr{bytePtr}, m_bitMask{bitMask} {}
        m_proxy operator*() const { return m_proxy(m_bytePtr, m_bitMask); }
        IteratorImpl operator++() {
            m_bitMask <<= 1;
            if (m_bitMask == 0) {
                m_bitMask = 1;
                m_bytePtr++;
            }
            return *this;
        }
        bool operator!=(const IteratorImpl& other) const {
            return m_bytePtr != other.m_bytePtr || m_bitMask != other.m_bitMask;
        }
    };

    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    dynamicArray() : m_count(0), m_byteCapacity(0), m_data(nullptr) {};  // default constructor

    dynamicArray(
        const std::initializer_list<bool>& list)  // overloaded constructor w/ default initialiser
        : m_count(list.size()),
          m_byteCapacity((list.size() + 7) / 8),
          m_data(std::make_unique<std::uint8_t[]>(m_byteCapacity)) {
        std::size_t globalIdx{0};
        for (bool val : list) {
            if (val) {
                m_data[calculateBucket(globalIdx)] |= calculateByteIndex(globalIdx);
            }
            globalIdx++;
        }
    }

    dynamicArray(const dynamicArray& other)  // copy constructor
        : m_count{other.m_count},
          m_byteCapacity{other.m_byteCapacity},
          m_data(std::make_unique<std::uint8_t[]>(m_byteCapacity)) {
        std::copy(other.m_data.get(), other.m_data.get() + other.m_byteCapacity, m_data.get());
    }

    dynamicArray& operator=(const dynamicArray& other) {
        if (this == &other) {
            return *this;
        }
        auto temp{std::make_unique<std::uint8_t[]>(other.m_byteCapacity)};
        std::copy(other.m_data.get(), other.m_data.get() + other.m_byteCapacity, temp.get());
        m_count = other.m_count;
        m_byteCapacity = other.m_byteCapacity;
        m_data = std::move(temp);
        return *this;
    }

    dynamicArray(dynamicArray&& other) noexcept
        : m_count(other.m_count),
          m_byteCapacity(other.m_byteCapacity),
          m_data(std::move(other.m_data)) {
        other.m_byteCapacity = 0;
        other.m_count = 0;
    }

    dynamicArray& operator=(dynamicArray&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        m_count = other.m_count;
        m_byteCapacity = other.m_byteCapacity;
        m_data = std::move(other.m_data);

        other.m_count = 0;
        other.m_byteCapacity = 0;

        return *this;
    }

    void push_back(const bool& val) {  // takes l value ref
        // capacity reached logic
        auto lastBucket = calculateBucket(m_count);
        if (lastBucket == m_byteCapacity && (m_count == 0 || m_data[lastBucket] == 0b1111'1111)) {
            std::cout << "reached capacity\n";
            m_byteCapacity = (m_count == 0) ? 1 : m_byteCapacity * 2;
            auto temp{std::make_unique<std::uint8_t[]>(m_byteCapacity)};
            std::copy(m_data.get(), m_data.get() + calculateBucket(m_count), temp.get());
            m_data = std::move(temp);
        }
        // insert logic
        m_count++;
        if (val) {
            m_data[lastBucket] |= calculateByteIndex(m_count);
        }
    }
    std::size_t size() const { return m_count; }
    std::size_t capacity() const { return m_byteCapacity; }
    Iterator begin() { return Iterator(m_data.get(), calculateByteIndex(0)); }
    Iterator end() {
        return Iterator(m_data.get() + calculateBucket(m_count), calculateByteIndex(m_count));
    }
    ConstIterator cbegin() const { return ConstIterator(m_data.get(), calculateByteIndex(0)); }
    ConstIterator cend() const {
        return ConstIterator(m_data.get() + calculateBucket(m_count), calculateByteIndex(m_count));
    }
    ConstIterator begin() const { return ConstIterator(m_data.get(), calculateByteIndex(0)); }
    ConstIterator end() const {
        return ConstIterator(m_data.get() + calculateBucket(m_count), calculateByteIndex(m_count));
    }

    Proxy operator[](std::size_t globalIndex) {  // proxy pattern to enable indexing
        return Proxy(&m_data[calculateBucket(globalIndex)], calculateByteIndex(globalIndex));
    }

    ConstProxy operator[](std::size_t globalIndex) const {  // proxy pattern to enable indexing
        return ConstProxy(&m_data[calculateBucket(globalIndex)], calculateByteIndex(globalIndex));
    }
};