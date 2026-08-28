#pragma once
#include <algorithm>
#include <cassert>
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
    std::unique_ptr<T[]> m_data;  // we must think about allocation here. this doesn't allow for
                                  // reserve to be implmeented i think.
                                  // friend class IteratorImpl;

   public:
    template <bool isConst>
    class IteratorImpl {
       private:
        using m_T = std::conditional_t<isConst, const T, T>;
        m_T* m_ptr;
        friend class IteratorImpl<!isConst>;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = m_T;
        using difference_type = std::ptrdiff_t;
        using reference = m_T&;
        using pointer = m_T*;

        IteratorImpl() : m_ptr(nullptr) {};

        IteratorImpl(m_T* ptr) : m_ptr(ptr) {};

        IteratorImpl(IteratorImpl& other) { m_ptr = other.m_ptr; }

        IteratorImpl& operator=(IteratorImpl& other) {
            if (*this == other) {
                return *this;
            }
            m_ptr = other.m_ptr;
            return *this;
        }

        IteratorImpl(IteratorImpl&& other) noexcept {
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }

        IteratorImpl& operator=(IteratorImpl&& other) noexcept {
            if (*this == other) {
                return *this;
            }
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
            return *this;
        }

        template <bool otherConst>
            requires(isConst && !otherConst)  // we only want non const to const
        IteratorImpl(const IteratorImpl<otherConst>& other) : m_ptr(other.m_ptr) {}

        m_T& operator*() { return *m_ptr; };

        IteratorImpl& operator++() {
            m_ptr++;
            return *this;
        }

        IteratorImpl operator++(int) {
            IteratorImpl temp = m_ptr;
            ++m_ptr;
            return temp;
        }

        // IteratorImpl* operator->() { return m_ptr; } think about this? only to work with custom
        // class and struct types?

        bool operator!=(const IteratorImpl& other) const { return m_ptr != other.m_ptr; }

        bool operator==(const IteratorImpl& other) const { return m_ptr == other.m_ptr; }
    };

    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    // combine the iterators above using std conditional like the bool specialisation
    // primary template iterators does not satisfy any iterator concept. work from input and output
    // interator assertions and work upward till we get to random access

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
    // think about self referencing bugs with push back. copy on grow instaed of moving is meh.
    // capacity means bytes in one class and elemnts in other.
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
    Iterator begin() { return Iterator(m_data.get()); }
    Iterator end() { return Iterator(m_data.get() + m_size); }
    ConstIterator cbegin() const noexcept { return begin(); }
    ConstIterator cend() const noexcept { return end(); }
    ConstIterator begin() const { return ConstIterator(m_data.get()); }
    ConstIterator end() const { return ConstIterator(m_data.get() + m_size); }

    // static_assert(std::random_access_iterator<Iterator>);

    // added this tells me what my iterator is missing
    // to satisfy being a random access iterator
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
        using iterator_category =
            std::forward_iterator_tag;  // fthis is currenty lying. this is an input iterator tag
                                        // pllus and random access on a seperat iterator_concept
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

    void push_back(bool val) {  // i think the condition here may be testing the wrong question
        // capacity reached logic
        if ((calculateBucket(m_count) == m_byteCapacity - 1 && calculateByteIndex(m_count) & 128) ||
            (m_byteCapacity == 0 && m_count == 0)) {
            auto newCapacity = (m_byteCapacity == 0) ? 1 : m_byteCapacity * 2;
            auto temp{std::make_unique<std::uint8_t[]>(newCapacity)};
            std::copy(m_data.get(), m_data.get() + m_byteCapacity, temp.get());
            m_byteCapacity = newCapacity;
            m_data = std::move(temp);
        }
        // insert logic
        if (val) {
            m_data[calculateBucket(m_count)] |= calculateByteIndex(m_count);
        }
        m_count++;
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