#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <atomic>

namespace matsimu {

/**
 * Bounded allocator: enforces a hard byte limit; throws std::bad_alloc on
 * exhaustion. No silent nullptr. Deterministic for same inputs and limit.
 *
 * Uses shared state for usage tracking across allocator copies.
 */
template <typename T, typename Inner = std::allocator<T>>
struct bounded_allocator {
  using value_type      = T;
  using inner_traits    = std::allocator_traits<Inner>;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  struct State {
    size_type max_bytes;
    std::atomic<size_type> current_bytes{0};
    explicit State(size_type m) : max_bytes(m) {}
  };

  Inner inner;
  std::shared_ptr<State> state;

  /// Default constructor: effectively unlimited (16 EB)
  bounded_allocator() noexcept 
    : state(std::make_shared<State>(static_cast<std::size_t>(-1))) {}

  explicit bounded_allocator(size_type max_bytes_, Inner in = {})
    : inner(std::move(in)), state(std::make_shared<State>(max_bytes_)) {}

  // Rebind constructor
  template <typename U>
  bounded_allocator(const bounded_allocator<U, typename inner_traits::template rebind_alloc<U>>& other) noexcept
    : inner(other.inner), state(other.state) {}

  T* allocate(size_type n) {
    if (n == 0) return nullptr;
    size_type need = n * sizeof(T);
    
    // Optimistic atomic update
    size_type current = state->current_bytes.load(std::memory_order_relaxed);
    while (true) {
        if (current + need > state->max_bytes)
            throw std::bad_alloc();
        if (state->current_bytes.compare_exchange_weak(current, current + need, 
                                                     std::memory_order_relaxed))
            break;
    }
    
    try {
        return inner_traits::allocate(inner, n);
    } catch (...) {
        state->current_bytes.fetch_sub(need, std::memory_order_relaxed);
        throw;
    }
  }

  void deallocate(T* p, size_type n) noexcept {
    if (!p) return;
    state->current_bytes.fetch_sub(n * sizeof(T), std::memory_order_relaxed);
    inner_traits::deallocate(inner, p, n);
  }

  template <typename U>
  struct rebind {
    using other = bounded_allocator<U, typename inner_traits::template rebind_alloc<U>>;
  };

  friend bool operator==(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return a.state == b.state && a.inner == b.inner;
  }
  friend bool operator!=(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return !(a == b);
  }
};

}  // namespace matsimu
