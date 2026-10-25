#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>

// Monotonic bump allocator designed for latency-critical paths.
// Eliminates kernel heap syscalls (malloc/free) by pre-allocating a contiguous buffer.
class Arena {
   private:
    std::byte* m_buffer{nullptr};
    std::size_t m_capacity{0};
    std::size_t m_offset{0};
    bool m_owns_buffer{false};

    static std::uintptr_t align_forward(std::uintptr_t ptr, std::size_t alignment) noexcept {
        assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of two");
        return (ptr + (alignment - 1)) & ~(alignment - 1);
    }

   public:
    // Construct with a pre-allocated external buffer (e.g. stack array or static buffer)
    Arena(void* buffer, std::size_t capacity) noexcept
        : m_buffer(static_cast<std::byte*>(buffer)),
          m_capacity(capacity),
          m_offset(0),
          m_owns_buffer(false) {}

    // Construct with dynamically allocated arena storage
    explicit Arena(std::size_t capacity)
        : m_buffer(new std::byte[capacity]),
          m_capacity(capacity),
          m_offset(0),
          m_owns_buffer(true) {}

    ~Arena() noexcept {
        if (m_owns_buffer) {
            delete[] m_buffer;
        }
    }

    // Non-copyable
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // Move-constructible
    Arena(Arena&& other) noexcept
        : m_buffer(other.m_buffer),
          m_capacity(other.m_capacity),
          m_offset(other.m_offset),
          m_owns_buffer(other.m_owns_buffer) {
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_offset = 0;
        other.m_owns_buffer = false;
    }

    Arena& operator=(Arena&& other) noexcept {
        if (this == &other) return *this;
        if (m_owns_buffer) {
            delete[] m_buffer;
        }
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_offset = other.m_offset;
        m_owns_buffer = other.m_owns_buffer;

        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_offset = 0;
        other.m_owns_buffer = false;
        return *this;
    }

    [[nodiscard]] void* allocate(std::size_t bytes,
                                 std::size_t alignment = alignof(std::max_align_t)) {
        if (bytes == 0) return nullptr;

        const std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(m_buffer + m_offset);
        const std::uintptr_t aligned_ptr = align_forward(current_ptr, alignment);
        const std::size_t padding = aligned_ptr - current_ptr;

        if (m_offset + padding + bytes > m_capacity) {
            throw std::bad_alloc();
        }

        m_offset += padding + bytes;
        return reinterpret_cast<void*>(aligned_ptr);
    }

    // Deallocation is a no-op in a monotonic arena
    void deallocate([[maybe_unused]] void* ptr, [[maybe_unused]] std::size_t bytes) noexcept {}

    // O(1) instant reset of the entire arena: reclaims all memory without free/delete overhead
    void reset() noexcept { m_offset = 0; }

    [[nodiscard]] std::size_t bytes_allocated() const noexcept { return m_offset; }
    [[nodiscard]] std::size_t bytes_remaining() const noexcept { return m_capacity - m_offset; }
    [[nodiscard]] std::size_t capacity() const noexcept { return m_capacity; }
    [[nodiscard]] const void* data() const noexcept { return m_buffer; }
};

// Fixed-capacity stack-backed arena helper
template <std::size_t N>
class StackArena : public Arena {
   private:
    alignas(std::max_align_t) std::byte m_storage[N];

   public:
    StackArena() noexcept : Arena(m_storage, N) {}
};

// Standard-compliant allocator model backed by Arena
template <typename T>
class ArenaAllocator {
   public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Allocator traits propagation rules
    using is_always_equal = std::false_type;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    template <typename U>
    struct rebind {
        using other = ArenaAllocator<U>;
    };

   private:
    Arena* m_arena{nullptr};

    template <typename U>
    friend class ArenaAllocator;

   public:
    ArenaAllocator() noexcept = default;

    explicit ArenaAllocator(Arena& arena) noexcept : m_arena(&arena) {}

    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) noexcept : m_arena(other.m_arena) {}

    ArenaAllocator(const ArenaAllocator& other) noexcept = default;
    ArenaAllocator& operator=(const ArenaAllocator& other) noexcept = default;

    [[nodiscard]] T* allocate(std::size_t n) {
        if (!m_arena) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(m_arena->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate([[maybe_unused]] T* ptr, [[maybe_unused]] std::size_t n) noexcept {
        // Monotonic arena deallocation is a no-op
    }

    [[nodiscard]] Arena* get_arena() const noexcept { return m_arena; }

    template <typename U>
    bool operator==(const ArenaAllocator<U>& other) const noexcept {
        return m_arena == other.m_arena;
    }

    template <typename U>
    bool operator!=(const ArenaAllocator<U>& other) const noexcept {
        return m_arena != other.m_arena;
    }
};
