#  Dynamic array implementation
## Purpose & non goals
This project is aimed to be an artifact to demonstate my understanding and application of key low level c++ programming techniques. To create a repilca of `std::vector`from the ground up, implementing features that are most relevant to my goals.

What I will not attempt:
- Positional insert/erase/emplace
- Reverse Iterators, assign, allocator extended constructors
- Small buffer optimisations
- Thread safety (yes really)

## Functional Requirements
### Construction / lifetime
| Operation | Time Complexity | Notes |
| --- | --- | --- |
| `dynamicArray()` | O(1) `noexcept` if allocators default ctor is | must not allocate |
| `explicit dynamicArray(size_type n)` | O(n) | value-init, needs `default_initializable<T>` |
| `dynamicArray(size_type n, const T&)` | O(n) |  |
|`dynamicArray(std::initializer_list<T>)` | O(n) | take by val |
|`template<input_iterator It> dynamicArray(It, it)` | O(n) single alloc if `sized_sentinel_for` | C++20 goodness |
| copy ctor / copy assign | O(n) |  |
| move ctor / move assign | O(1) `noexcept` | |
| destructor | O(n) | |

### Capacity
| Operation | Time Complexity | Notes |
| --- | --- | --- |
| `size()`, `capacity()`, `empty()`, `max_size()` | all O(1) `noexcept`, `[[nodiscard]] | |
| `reserve(n)` | O(n) | Linear on growth, no-op if `n <= capacity()`, never shrinks and invalidates everything on reallocation.| 
| `shrink_to_fit()` | O(n) / O(1) if does nothing | non-binding, ignore if `capacity()` is equal to `size()`

### Access
| Operation | Time Complexity | Notes |
| --- | --- | --- |
| `operator[]`, `front()`, `back()`, `data()` | all O(1) `noexcept` | preconditioned check via `assert` only |
| `at()` | O(1) | throws `std::out_of_range` (seperate to subscript op which must be low latency path)| 

### Modifiers
| Operation | Time Complexity | Notes |
| --- | --- | --- |
| `push_back(const T&)`, `push_back(T&&)`, `emplace_back(Args&&...)` | amortised O(1)  | implement `emplace_back` as the primitive and have both `push_back`s delegate |
| `pop_back()` | O(1) `noexcept` | preconditioned `!empty()` |
| `clear()` | O(n) `noexcept` | capacity unchanged |
| `resize(n)`, `resize(n, const T&)` | O(abs(n-`size()`)) | | 
| `swap(dynamicArray&) noexcept` + free swap by arg deduction | O(1) | |
| `operator==`, `operator<=>` | Equality O(n)/O(1) / Three way Ordering 0(1)/O(N) | equality can early return based off size mismatch / three way ordering must check elements |

### Iterator Categories
- `std::contiguous_iterator` for primary template
- boolean class specialisation (must split the two type aliases):
- `iterator_category = std::input_iterator_tag`
- Target: `iterator_concept = std::random_access_iterator_tag`: will need to explain the taxonomy split

## Non-Functional Requirements

### Exception safety

Stated per operation. The guarantee is part of the contract, not an
implementation detail.

| Operation | Guarantee |
| --- | --- |
| `push_back`, `emplace_back` | **Strong** — container unchanged if the operation throws, provided `T`'s move ctor is `noexcept` or `T` is copy-constructible. Same condition `std::vector` uses. |
| copy assignment | **Strong** — allocate and copy into new storage, commit only on success. |
| `reserve` | **Strong** — same relocation condition as above. |
| move ctor, move assign, `swap`, `clear`, `pop_back` | `noexcept` |
| everything else | **Basic** — no leaks, all invariants hold, object remains destructible and assignable. |
| destructors | Never throw. |

The condition on `push_back` is the reason `std::move_if_noexcept` exists.
If relocation moves element *k* and the move throws, elements `[0, k)` in
the source are already gutted and there is no way to restore them — so
unless moves are known not to throw, relocation must copy, which leaves
the source intact and lets us unwind cleanly.

### Allocator awareness

- `template <typename T, typename Allocator = std::allocator<T>>`.
- All allocation, deallocation, construction and destruction goes through
  `std::allocator_traits<Allocator>` — never through allocator members
  directly, since traits supply defaults for anything the allocator omits.
- Allocator stored as a `[[no_unique_address]]` member so a stateless
  allocator costs zero bytes.
- Respect `propagate_on_container_copy_assignment`,
  `propagate_on_container_move_assignment`, and
  `propagate_on_container_swap`.
- Over-aligned `T` must not be silently mis-aligned. `std::allocator<T>`
  handles this; hand-rolled raw allocation would need
  `::operator new(n, std::align_val_t{alignof(T)})`.

Motivation: arena, pool, huge-page-backed and NUMA-pinned allocators are
how latency-sensitive systems control allocation. A container that hard-codes
`new` cannot participate in that. A monotonic arena allocator is included
in this repo as a benchmark driver to demonstrate the design generalises.

### Memory layout and cache behaviour

What "low latency" actually constrains here — specific claims only,
each one measured in the benchmark suite:

1. **Member layout.** Three pointers (`begin`, `end`, `cap_end`) rather than
   pointer plus two `size_t`. `end()` becomes a load instead of a load-add,
   and `push_back`'s capacity check becomes a pointer compare. Benchmarked
   against the index-based layout.
2. **Hot/cold split.** The reallocation path is extracted into a separate
   `[[gnu::noinline]]` function. It runs once per ~n pushes; inlining it
   bloats the fast path at every call site and hurts I-cache pressure and
   inlining decisions elsewhere.
3. **Growth factor.** With 2×, the sum of all previously freed blocks is
   always strictly less than the next request, so freed blocks can never be
   reused. 1.5x has it's pro's also but not sure about which one to go with for now. Will follow up in the design and decisions section
4. **`operator[]` does not branch in release builds.** Preconditions are
   checked with `assert`, which compiles out under `NDEBUG`. Callers wanting
   a checked access use `at()`.
5. **Bit packing in the `<bool>` specialisation** trades 8× less memory
   traffic for shift-and-mask work per element. Which side wins depends on
   `n` relative to L1/L2/LLC and on the cost of the per-element operation.
   Characterising that crossover is the headline benchmark.

### Thread safety

The standard container contract, and nothing more: concurrent reads through
`const` access are safe; any non-`const` access requires external
synchronisation. No internal locking, no atomics. A container that
synchronises internally is a different and worse design — the caller
almost always knows better where the boundary belongs.

### Other

- `constexpr`-enabled throughout. C++20 permits allocation during constant
  evaluation, so construction, `push_back` and iteration are verified at
  compile time by `static_assert`. This also serves as a UB check: the
  constant evaluator rejects code the runtime would silently accept.
- The header does not include `<iostream>` or any other heavyweight or
  unused header. A library header should not drag a static initialiser into
  every translation unit that touches it.
- All observers are `[[nodiscard]]` and `noexcept`.


## Invariants

Hold on entry to and exit from every public function, including when an
exception propagates out of one.

1. `size <= capacity`
2. `capacity == 0` ⟺ no storage owned
3. if `m_data != nullptr` it points to a single allocation of `m_capacity` objets worth of storage
4. Only the elements in range `[0, m_size)` are alive, `[m_size,  m_capacity)` is raw storage with no live objs
5. `m_data[0.. m_size)` are contiguous with no padding beyond `sizeof(T)`
6. `begin() + m_size == end()`, with range always being valid to travese
7. An moved-from instance of this container satisfies all of the above, `empty()`as true 
8. Every publicly exposed function is either no-op or leaves inv's 1-7 intact, including on the exceptional path
9. Reallocation invalidates all iterators, pointers and references, `pop_back/clear` invalidates those at or past the erased elements. `reserve(n)` where `n <= capacity()` must not invalidate resources
10. For `<bool>` - `m_count <= m_byteCapacity * 8` and all bits in range `[m_count, m_byteCapacity * 8)` are 0

## Design decisions & rationale 
— will add to this as i go