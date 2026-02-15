---
name: resource-aware-cpp-allocator
description: Design and implement resource-aware custom allocators in C++ with explicit limits, pool/arena strategies, and fail-fast OOM handling. Use when implementing or refactoring C++ memory allocation, optimizing for constrained environments (edge, embedded), or when the user mentions allocators, memory pools, arena allocation, or resource limits in C++.
---

# Resource-Aware C++ Allocator

Guides the agent to implement or recommend C++ allocators that enforce resource limits, avoid silent failures, and fit resource-conscious codebases (e.g. edge-ready, embedded, or simulation workloads).

## When to Apply

- User asks for custom allocators, memory pools, or arena allocators in C++
- Code needs hard memory caps or predictable allocation behavior
- Refactoring from raw `new`/`delete` or unbounded containers in resource-constrained contexts
- Project rules stress "resource consciousness" or "minimal RAM/CPU"

## Core Principles

1. **Explicit limits**: Enforce a maximum allocation budget in code; do not rely on "reasonable" usage.
2. **Fail fast**: On exhaustion, report an error or throw; never return `nullptr` and pretend success.
3. **Determinism**: Allocation success/failure must be reproducible for the same inputs and limits.
4. **Minimal surface**: Prefer wrapping the standard allocator or `std::allocator_traits` rather than reimplementing the full Allocator interface from scratch.

## Allocator Requirements (C++11/17)

A custom allocator must satisfy the allocator requirements used by the standard library (e.g. for `std::vector<T, Alloc>`, `std::map`). Use `std::allocator_traits<Alloc>` for:

- `allocate(Alloc& a, size_type n)` → may throw or return nullptr only if the allocator documents it and callers check
- `deallocate(Alloc& a, pointer p, size_type n)` → no-throw
- `rebind_alloc<U>` (or `rebind` in C++11) for containers that allocate internal node types

For resource-aware allocators, **do not** document "return nullptr on failure" unless every caller is updated to check; prefer throwing `std::bad_alloc` (or a project-specific exception) on limit exceeded.

## Patterns

### 1. Bounded allocator (wrapper)

Wrap an existing allocator (e.g. `std::allocator<T>`) and enforce a hard cap:

- Maintain a `current_usage` (and optionally `peak_usage`) in bytes or in number of blocks.
- In `allocate(n)`: if `current_usage + n * sizeof(T) > max_bytes`, throw `std::bad_alloc` (or a custom exception); otherwise update `current_usage` and forward to the inner allocator.
- In `deallocate(p, n)`: subtract from `current_usage`, then forward to the inner allocator.
- Thread safety: if the allocator is used from multiple threads, protect the counter with a mutex or use atomic operations; document whether the allocator is thread-safe.

### 2. Pool allocator (fixed-size blocks)

Use when many small objects of the same size are allocated and freed:

- Preallocate a contiguous block (or use a list of blocks) and hand out fixed-size chunks.
- Enforces a natural limit by the number of preallocated chunks.
- On exhaustion, either throw or return nullptr **and** document that callers must check (prefer throw for consistency with "no silent failure").

### 3. Arena / linear allocator

Use when allocations have stack-like lifetime (e.g. per simulation step):

- One or more large buffers; allocate by advancing a pointer; no per-block `deallocate` (or only support clearing the whole arena).
- Hard limit is the arena size; after that, throw.
- Good for batch processing where all allocations in a phase can be discarded at once.

## Integration with STL

- `std::vector<T, Alloc>`, `std::string`, `std::map`/`std::set` (with node-based allocators) accept an allocator template parameter.
- Pass a resource-aware allocator instance when constructing the container; the container will use it for all internal allocations.
- Ensure the allocator is **copyable** as required by the standard (containers may copy the allocator).

## Invariants to Enforce

- **No silent OOM**: Exceeding the limit must be visible (throw or explicit error path), not a silent nullptr.
- **Limit in code**: The maximum size or count is a constant or configuration value checked in the allocator, not only in documentation.
- **Determinism**: Same inputs and same limit must yield the same pass/fail behavior; avoid hidden randomness in allocation order or sizing.

## Anti-Patterns

- **Returning nullptr without enforcing checks**: Leads to silent corruption; prefer throwing.
- **Unbounded growth**: "Pool" or "arena" that can grow without limit defeats resource awareness.
- **Raw `new`/`delete` for bulk or repeated allocation**: Prefer a single backing allocation (e.g. vector or custom buffer) plus a resource-aware allocator that parcels it out.
- **Documentation-only limits**: Stating "don't allocate more than X" without enforcing X in code violates explicit-over-implicit.

## Minimal Example: Bounded wrapper

```cpp
template <typename T, typename Inner = std::allocator<T>>
struct bounded_allocator {
  using value_type = T;
  using inner_traits = std::allocator_traits<Inner>;

  Inner inner;
  std::size_t max_bytes;
  std::size_t current_bytes{0};

  explicit bounded_allocator(std::size_t max_bytes, Inner in = {})
    : inner(std::move(in)), max_bytes(max_bytes) {}

  T* allocate(std::size_t n) {
    std::size_t need = n * sizeof(T);
    if (current_bytes + need > max_bytes)
      throw std::bad_alloc();
    current_bytes += need;
    return inner_traits::allocate(inner, n);
  }

  void deallocate(T* p, std::size_t n) noexcept {
    current_bytes -= n * sizeof(T);
    inner_traits::deallocate(inner, p, n);
  }

  template <typename U>
  struct rebind {
    using other = bounded_allocator<U, typename std::allocator_traits<Inner>::template rebind_alloc<U>>;
  };

  friend bool operator==(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return a.max_bytes == b.max_bytes && a.inner == b.inner;
  }
  friend bool operator!=(const bounded_allocator& a, const bounded_allocator& b) noexcept {
    return !(a == b);
  }
};

// Usage: std::vector<int, bounded_allocator<int>> v(bounded_allocator<int>(4096));
```

## Summary Checklist

When adding or reviewing a resource-aware allocator:

- [ ] Hard limit (bytes or count) is enforced in code.
- [ ] On limit exceeded, the code throws or returns an explicit error (no silent nullptr unless documented and every caller checks).
- [ ] Allocator is copyable and compatible with `std::allocator_traits` (and rebind if used with node-based containers).
- [ ] Thread safety (or lack thereof) is documented.
- [ ] No unbounded growth; pools/arenas have fixed capacity or a configured maximum.
