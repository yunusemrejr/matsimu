#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

namespace matsimu {

/**
 * Bounded allocator: enforces a hard byte limit; throws std::bad_alloc on
 * exhaustion. No silent nullptr. Deterministic for same inputs and limit.
 *
 * Not thread-safe unless inner allocator is thread-safe and you synchronise
 * access to this allocator instance.
 */
template <typename T, typename Inner = std::allocator<T>>
struct bounded_allocator {
  using value_type      = T;
  using inner_traits    = std::allocator_traits<Inner>;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  Inner inner;
  size_type max_bytes;
  size_type current_bytes{0};

  explicit bounded_allocator(size_type max_bytes_, Inner in = {})
    : inner(std::move(in)), max_bytes(max_bytes_) {}

  T* allocate(size_type n) {
    size_type need = n * sizeof(T);
    if (current_bytes + need > max_bytes)
      throw std::bad_alloc();
    current_bytes += need;
    return inner_traits::allocate(inner, n);
  }

  void deallocate(T* p, size_type n) noexcept {
    current_bytes -= n * sizeof(T);
    inner_traits::deallocate(inner, p, n);
  }

  template <typename U>
  struct rebind {
    using other = bounded_allocator<U, typename inner_traits::template rebind_alloc<U>>;
  };

  friend bool operator==(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return a.max_bytes == b.max_bytes && a.current_bytes == b.current_bytes && a.inner == b.inner;
  }
  friend bool operator!=(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return !(a == b);
  }
};

}  // namespace matsimu
