#pragma once
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T, typename Alloc = std::allocator<T>>
class dynamicArray {
   public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;

   private:
    using AllocatorTraits = std::allocator_traits<Alloc>;
    using pointer = typename AllocatorTraits::pointer;
    using const_pointer = typename AllocatorTraits::const_pointer;

    // 3-pointer layout: begin, end, cap_end
    // end() is a single pointer load instead of load-add.
    // push_back capacity check is a single pointer compare.
    pointer m_begin{nullptr};
    pointer m_end{nullptr};
    pointer m_cap{nullptr};
    [[no_unique_address]] Alloc m_alloc{};

    void destroy_range(pointer first, pointer last) noexcept {
        for (; first != last; ++first) {
            AllocatorTraits::destroy(m_alloc, std::to_address(first));
        }
    }

    void deallocate_buffer() noexcept {
        if (m_begin) {
            AllocatorTraits::deallocate(m_alloc, m_begin, capacity());
            m_begin = m_end = m_cap = nullptr;
        }
    }

    template <typename... Args>
    [[gnu::noinline]] reference emplace_back_slow_path(Args&&... args) {
        const size_type cur_cap = capacity();
        const size_type new_cap = (cur_cap == 0) ? 1 : cur_cap * 2;
        const size_type old_size = size();

        pointer new_buffer = AllocatorTraits::allocate(m_alloc, new_cap);
        pointer new_element_pos = new_buffer + old_size;

        // Construct new element first: guarantees self-reference safety (e.g. v.push_back(v[0]))
        // and strong exception safety (old buffer untouched if constructor throws).
        try {
            AllocatorTraits::construct(m_alloc, std::to_address(new_element_pos),
                                       std::forward<Args>(args)...);
        } catch (...) {
            AllocatorTraits::deallocate(m_alloc, new_buffer, new_cap);
            throw;
        }

        // Relocate existing elements to new buffer
        pointer new_end = new_buffer;
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                AllocatorTraits::construct(m_alloc, std::to_address(new_end), std::move(*src));
            }
        } else {
            try {
                for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(new_end), *src);
                }
            } catch (...) {
                destroy_range(new_buffer, new_end);
                AllocatorTraits::destroy(m_alloc, std::to_address(new_element_pos));
                AllocatorTraits::deallocate(m_alloc, new_buffer, new_cap);
                throw;
            }
        }

        destroy_range(m_begin, m_end);
        if (m_begin) {
            AllocatorTraits::deallocate(m_alloc, m_begin, cur_cap);
        }

        m_begin = new_buffer;
        m_end = new_element_pos + 1;
        m_cap = new_buffer + new_cap;

        return *new_element_pos;
    }

   public:
    template <bool isConst>
    class IteratorImpl {
       private:
        using m_T = std::conditional_t<isConst, const T, T>;
        m_T* m_ptr;
        friend class IteratorImpl<!isConst>;
        friend class dynamicArray;

       public:
        using iterator_concept = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = m_T&;
        using pointer = m_T*;

        IteratorImpl() noexcept : m_ptr(nullptr) {}

        explicit IteratorImpl(m_T* ptr) noexcept : m_ptr(ptr) {}

        IteratorImpl(const IteratorImpl& other) noexcept : m_ptr(other.m_ptr) {}

        IteratorImpl& operator=(const IteratorImpl& other) noexcept {
            m_ptr = other.m_ptr;
            return *this;
        }

        template <bool otherConst>
            requires(isConst && !otherConst)
        IteratorImpl(const IteratorImpl<otherConst>& other) noexcept : m_ptr(other.m_ptr) {}

        m_T& operator*() const noexcept { return *m_ptr; }

        pointer operator->() const noexcept { return m_ptr; }

        IteratorImpl& operator++() noexcept {
            ++m_ptr;
            return *this;
        }

        IteratorImpl operator++(int) noexcept {
            IteratorImpl temp = *this;
            ++m_ptr;
            return temp;
        }

        IteratorImpl& operator--() noexcept {
            --m_ptr;
            return *this;
        }

        IteratorImpl operator--(int) noexcept {
            IteratorImpl temp = *this;
            --m_ptr;
            return temp;
        }

        IteratorImpl& operator+=(difference_type n) noexcept {
            m_ptr += n;
            return *this;
        }

        IteratorImpl& operator-=(difference_type n) noexcept {
            m_ptr -= n;
            return *this;
        }

        IteratorImpl operator+(difference_type n) const noexcept { return IteratorImpl(m_ptr + n); }

        friend IteratorImpl operator+(difference_type n, const IteratorImpl& it) noexcept {
            return IteratorImpl(it.m_ptr + n);
        }

        IteratorImpl operator-(difference_type n) const noexcept { return IteratorImpl(m_ptr - n); }

        template <bool otherConst>
        difference_type operator-(const IteratorImpl<otherConst>& other) const noexcept {
            return m_ptr - other.m_ptr;
        }

        reference operator[](difference_type n) const noexcept { return m_ptr[n]; }

        template <bool otherConst>
        bool operator==(const IteratorImpl<otherConst>& other) const noexcept {
            return m_ptr == other.m_ptr;
        }

        template <bool otherConst>
        auto operator<=>(const IteratorImpl<otherConst>& other) const noexcept {
            return m_ptr <=> other.m_ptr;
        }
    };

    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;
    using iterator = Iterator;
    using const_iterator = ConstIterator;

    static_assert(std::random_access_iterator<Iterator>);
    static_assert(std::contiguous_iterator<Iterator>);
    static_assert(std::random_access_iterator<ConstIterator>);
    static_assert(std::contiguous_iterator<ConstIterator>);

    // Constructors & Destructor
    dynamicArray() noexcept(std::is_nothrow_default_constructible_v<Alloc>)
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc() {}

    explicit dynamicArray(const Alloc& alloc) noexcept
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc(alloc) {}

    explicit dynamicArray(size_type count, const Alloc& alloc = Alloc())
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc(alloc) {
        if (count > 0) {
            m_begin = AllocatorTraits::allocate(m_alloc, count);
            m_cap = m_begin + count;
            m_end = m_begin;
            try {
                for (; m_end != m_cap; ++m_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(m_end));
                }
            } catch (...) {
                destroy_range(m_begin, m_end);
                AllocatorTraits::deallocate(m_alloc, m_begin, count);
                m_begin = m_end = m_cap = nullptr;
                throw;
            }
        }
    }

    dynamicArray(size_type count, const T& value, const Alloc& alloc = Alloc())
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc(alloc) {
        if (count > 0) {
            m_begin = AllocatorTraits::allocate(m_alloc, count);
            m_cap = m_begin + count;
            m_end = m_begin;
            try {
                for (; m_end != m_cap; ++m_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(m_end), value);
                }
            } catch (...) {
                destroy_range(m_begin, m_end);
                AllocatorTraits::deallocate(m_alloc, m_begin, count);
                m_begin = m_end = m_cap = nullptr;
                throw;
            }
        }
    }

    dynamicArray(std::initializer_list<T> init, const Alloc& alloc = Alloc())
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc(alloc) {
        const size_type count = init.size();
        if (count > 0) {
            m_begin = AllocatorTraits::allocate(m_alloc, count);
            m_cap = m_begin + count;
            m_end = m_begin;
            try {
                for (const auto& elem : init) {
                    AllocatorTraits::construct(m_alloc, std::to_address(m_end), elem);
                    ++m_end;
                }
            } catch (...) {
                destroy_range(m_begin, m_end);
                AllocatorTraits::deallocate(m_alloc, m_begin, count);
                m_begin = m_end = m_cap = nullptr;
                throw;
            }
        }
    }

    dynamicArray(const dynamicArray& other)
        : m_begin(nullptr),
          m_end(nullptr),
          m_cap(nullptr),
          m_alloc(AllocatorTraits::select_on_container_copy_construction(other.m_alloc)) {
        const size_type count = other.size();
        if (count > 0) {
            m_begin = AllocatorTraits::allocate(m_alloc, count);
            m_cap = m_begin + count;
            m_end = m_begin;
            try {
                for (pointer src = other.m_begin; src != other.m_end; ++src, ++m_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(m_end), *src);
                }
            } catch (...) {
                destroy_range(m_begin, m_end);
                AllocatorTraits::deallocate(m_alloc, m_begin, count);
                m_begin = m_end = m_cap = nullptr;
                throw;
            }
        }
    }

    dynamicArray(dynamicArray&& other) noexcept
        : m_begin(other.m_begin),
          m_end(other.m_end),
          m_cap(other.m_cap),
          m_alloc(std::move(other.m_alloc)) {
        other.m_begin = nullptr;
        other.m_end = nullptr;
        other.m_cap = nullptr;
    }

    dynamicArray(dynamicArray&& other, const Alloc& alloc)
        : m_begin(nullptr), m_end(nullptr), m_cap(nullptr), m_alloc(alloc) {
        if (m_alloc == other.m_alloc) {
            m_begin = other.m_begin;
            m_end = other.m_end;
            m_cap = other.m_cap;
            other.m_begin = other.m_end = other.m_cap = nullptr;
        } else {
            const size_type count = other.size();
            if (count > 0) {
                m_begin = AllocatorTraits::allocate(m_alloc, count);
                m_cap = m_begin + count;
                m_end = m_begin;
                try {
                    for (pointer src = other.m_begin; src != other.m_end; ++src, ++m_end) {
                        AllocatorTraits::construct(m_alloc, std::to_address(m_end),
                                                   std::move(*src));
                    }
                } catch (...) {
                    destroy_range(m_begin, m_end);
                    AllocatorTraits::deallocate(m_alloc, m_begin, count);
                    m_begin = m_end = m_cap = nullptr;
                    throw;
                }
            }
            other.clear();
        }
    }

    ~dynamicArray() noexcept {
        destroy_range(m_begin, m_end);
        deallocate_buffer();
    }

    dynamicArray& operator=(const dynamicArray& other) {
        if (this == &other) {
            return *this;
        }

        if constexpr (AllocatorTraits::propagate_on_container_copy_assignment::value) {
            if (m_alloc != other.m_alloc) {
                clear();
                deallocate_buffer();
                m_alloc = other.m_alloc;
            }
        }

        const size_type other_size = other.size();
        if (other_size > capacity()) {
            pointer new_buffer = AllocatorTraits::allocate(m_alloc, other_size);
            pointer new_end = new_buffer;
            try {
                for (pointer src = other.m_begin; src != other.m_end; ++src, ++new_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(new_end), *src);
                }
            } catch (...) {
                destroy_range(new_buffer, new_end);
                AllocatorTraits::deallocate(m_alloc, new_buffer, other_size);
                throw;
            }

            destroy_range(m_begin, m_end);
            deallocate_buffer();

            m_begin = new_buffer;
            m_end = new_end;
            m_cap = new_buffer + other_size;
        } else {
            const size_type cur_size = size();
            if (cur_size >= other_size) {
                pointer cur = m_begin;
                pointer src = other.m_begin;
                for (; src != other.m_end; ++cur, ++src) {
                    *cur = *src;
                }
                destroy_range(cur, m_end);
                m_end = m_begin + other_size;
            } else {
                pointer cur = m_begin;
                pointer src = other.m_begin;
                for (; cur != m_end; ++cur, ++src) {
                    *cur = *src;
                }
                for (; src != other.m_end; ++src, ++m_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(m_end), *src);
                }
            }
        }
        return *this;
    }

    dynamicArray& operator=(dynamicArray&& other) noexcept(
        AllocatorTraits::propagate_on_container_move_assignment::value ||
        AllocatorTraits::is_always_equal::value) {
        if (this == &other) {
            return *this;
        }

        constexpr bool propagate = AllocatorTraits::propagate_on_container_move_assignment::value;
        if constexpr (propagate || AllocatorTraits::is_always_equal::value) {
            destroy_range(m_begin, m_end);
            deallocate_buffer();
            if constexpr (propagate) {
                m_alloc = std::move(other.m_alloc);
            }
            m_begin = other.m_begin;
            m_end = other.m_end;
            m_cap = other.m_cap;
            other.m_begin = other.m_end = other.m_cap = nullptr;
        } else {
            if (m_alloc == other.m_alloc) {
                destroy_range(m_begin, m_end);
                deallocate_buffer();
                m_begin = other.m_begin;
                m_end = other.m_end;
                m_cap = other.m_cap;
                other.m_begin = other.m_end = other.m_cap = nullptr;
            } else {
                clear();
                reserve(other.size());
                for (pointer src = other.m_begin; src != other.m_end; ++src) {
                    emplace_back(std::move(*src));
                }
                other.clear();
            }
        }
        return *this;
    }

    // Modifiers
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        if (m_end != m_cap) [[likely]] {
            AllocatorTraits::construct(m_alloc, std::to_address(m_end),
                                       std::forward<Args>(args)...);
            reference ref = *m_end;
            ++m_end;
            return ref;
        }
        return emplace_back_slow_path(std::forward<Args>(args)...);
    }

    void push_back(const T& x) { emplace_back(x); }

    void push_back(T&& x) { emplace_back(std::move(x)); }

    void pop_back() noexcept {
        assert(!empty() && "dynamicArray::pop_back: container is empty");
        --m_end;
        AllocatorTraits::destroy(m_alloc, std::to_address(m_end));
    }

    void clear() noexcept {
        destroy_range(m_begin, m_end);
        m_end = m_begin;
    }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity()) {
            return;
        }
        pointer new_buffer = AllocatorTraits::allocate(m_alloc, new_cap);
        pointer new_end = new_buffer;
        const size_type cur_cap = capacity();

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                AllocatorTraits::construct(m_alloc, std::to_address(new_end), std::move(*src));
            }
        } else {
            try {
                for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(new_end), *src);
                }
            } catch (...) {
                destroy_range(new_buffer, new_end);
                AllocatorTraits::deallocate(m_alloc, new_buffer, new_cap);
                throw;
            }
        }

        destroy_range(m_begin, m_end);
        if (m_begin) {
            AllocatorTraits::deallocate(m_alloc, m_begin, cur_cap);
        }

        const size_type old_size = size();
        m_begin = new_buffer;
        m_end = new_buffer + old_size;
        m_cap = new_buffer + new_cap;
    }

    void shrink_to_fit() {
        const size_type cur_size = size();
        if (cur_size == capacity()) {
            return;
        }
        if (cur_size == 0) {
            destroy_range(m_begin, m_end);
            deallocate_buffer();
            return;
        }

        pointer new_buffer = AllocatorTraits::allocate(m_alloc, cur_size);
        pointer new_end = new_buffer;
        const size_type cur_cap = capacity();

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                AllocatorTraits::construct(m_alloc, std::to_address(new_end), std::move(*src));
            }
        } else {
            try {
                for (pointer src = m_begin; src != m_end; ++src, ++new_end) {
                    AllocatorTraits::construct(m_alloc, std::to_address(new_end), *src);
                }
            } catch (...) {
                destroy_range(new_buffer, new_end);
                AllocatorTraits::deallocate(m_alloc, new_buffer, cur_size);
                throw;
            }
        }

        destroy_range(m_begin, m_end);
        if (m_begin) {
            AllocatorTraits::deallocate(m_alloc, m_begin, cur_cap);
        }

        m_begin = new_buffer;
        m_end = new_end;
        m_cap = new_buffer + cur_size;
    }

    void resize(size_type new_size) {
        const size_type cur_size = size();
        if (new_size < cur_size) {
            destroy_range(m_begin + new_size, m_end);
            m_end = m_begin + new_size;
        } else if (new_size > cur_size) {
            if (new_size > capacity()) {
                reserve(new_size);
            }
            for (; m_end != m_begin + new_size; ++m_end) {
                AllocatorTraits::construct(m_alloc, std::to_address(m_end));
            }
        }
    }

    void resize(size_type new_size, const T& value) {
        const size_type cur_size = size();
        if (new_size < cur_size) {
            destroy_range(m_begin + new_size, m_end);
            m_end = m_begin + new_size;
        } else if (new_size > cur_size) {
            if (new_size > capacity()) {
                reserve(new_size);
            }
            for (; m_end != m_begin + new_size; ++m_end) {
                AllocatorTraits::construct(m_alloc, std::to_address(m_end), value);
            }
        }
    }

    void swap(dynamicArray& other) noexcept {
        using std::swap;
        swap(m_begin, other.m_begin);
        swap(m_end, other.m_end);
        swap(m_cap, other.m_cap);
        if constexpr (AllocatorTraits::propagate_on_container_swap::value) {
            swap(m_alloc, other.m_alloc);
        }
    }

    friend void swap(dynamicArray& a, dynamicArray& b) noexcept { a.swap(b); }

    // Observers
    [[nodiscard]] size_type size() const noexcept {
        return static_cast<size_type>(m_end - m_begin);
    }

    [[nodiscard]] size_type capacity() const noexcept {
        return static_cast<size_type>(m_cap - m_begin);
    }

    [[nodiscard]] bool empty() const noexcept { return m_begin == m_end; }

    [[nodiscard]] size_type max_size() const noexcept { return AllocatorTraits::max_size(m_alloc); }

    [[nodiscard]] allocator_type get_allocator() const noexcept { return m_alloc; }

    // Element Access
    [[nodiscard]] reference operator[](size_type index) noexcept {
        assert(index < size() && "dynamicArray::operator[]: index out of bounds");
        return m_begin[index];
    }

    [[nodiscard]] const_reference operator[](size_type index) const noexcept {
        assert(index < size() && "dynamicArray::operator[]: index out of bounds");
        return m_begin[index];
    }

    [[nodiscard]] reference at(size_type index) {
        if (index >= size()) {
            throw std::out_of_range("dynamicArray::at: index out of range");
        }
        return m_begin[index];
    }

    [[nodiscard]] const_reference at(size_type index) const {
        if (index >= size()) {
            throw std::out_of_range("dynamicArray::at: index out of range");
        }
        return m_begin[index];
    }

    [[nodiscard]] reference front() noexcept {
        assert(!empty() && "dynamicArray::front: container is empty");
        return *m_begin;
    }

    [[nodiscard]] const_reference front() const noexcept {
        assert(!empty() && "dynamicArray::front: container is empty");
        return *m_begin;
    }

    [[nodiscard]] reference back() noexcept {
        assert(!empty() && "dynamicArray::back: container is empty");
        return *(m_end - 1);
    }

    [[nodiscard]] const_reference back() const noexcept {
        assert(!empty() && "dynamicArray::back: container is empty");
        return *(m_end - 1);
    }

    [[nodiscard]] pointer data() noexcept { return m_begin; }
    [[nodiscard]] const_pointer data() const noexcept { return m_begin; }

    // Iterators
    [[nodiscard]] Iterator begin() noexcept { return Iterator(m_begin); }
    [[nodiscard]] Iterator end() noexcept { return Iterator(m_end); }
    [[nodiscard]] ConstIterator begin() const noexcept { return ConstIterator(m_begin); }
    [[nodiscard]] ConstIterator end() const noexcept { return ConstIterator(m_end); }
    [[nodiscard]] ConstIterator cbegin() const noexcept { return ConstIterator(m_begin); }
    [[nodiscard]] ConstIterator cend() const noexcept { return ConstIterator(m_end); }

    // Comparisons
    bool operator==(const dynamicArray& other) const {
        if (size() != other.size()) return false;
        return std::equal(m_begin, m_end, other.m_begin);
    }

    auto operator<=>(const dynamicArray& other) const
        requires requires(const T& a, const T& b) { a <=> b; }
    {
        return std::lexicographical_compare_three_way(m_begin, m_end, other.m_begin, other.m_end);
    }
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