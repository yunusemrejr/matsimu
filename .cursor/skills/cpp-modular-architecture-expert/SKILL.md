---
name: cpp-modular-architecture-expert
description: Designs and reviews modular C++ architecture: interface/implementation separation, header organization, dependency boundaries, and DRY structure. Use when designing or refactoring C++ modules, reviewing C++ architecture, adding components, or when the user mentions modular C++, headers, clean C++ structure, or dependency layout.
---

# C++ Modular Architecture Expert

## Quick Start

When designing or reviewing C++ structure:

1. **Interface vs implementation** — Public API in headers; implementation in `.cpp` or inline only when small and stable.
2. **One responsibility per module** — One coherent abstraction per header (or small, related group). Avoid kitchen-sink headers.
3. **Dependency direction** — Depend inward only. No circular dependencies. Lower-level modules do not depend on higher-level ones.
4. **DRY** — Shared logic lives in one place (shared header or source). No duplicated logic across translation units.

## Interface vs Implementation

- **Headers** (`.h` / `.hpp`): Declarations, inline/constexpr/template definitions that are part of the interface or must be visible to the compiler.
- **Source** (`.cpp`): Non-inline definitions, internal helpers, anything that would pollute the public API or increase compile time unnecessarily.

Prefer putting only what callers need in headers. Move large or frequently changing implementation details to `.cpp` so changes don’t force full rebuilds.

```cpp
// good: header declares; source defines
// foo.hpp
namespace bar {
void do_work(const Input& in, Output& out);
}

// foo.cpp
void bar::do_work(const Input& in, Output& out) { /* ... */ }
```

For small, stable, performance-critical code, header-only with `inline` / `constexpr` is acceptable. Prefer one approach per module: either “thin header + .cpp” or “header-only,” and stick to it.

## Header Organization

- **Include guards** — Use `#pragma once` or classic `#ifndef FOO_HPP` / `#define` / `#endif` consistently.
- **Minimal includes** — Include only what the header needs. Prefer forward declarations in headers and include full types in `.cpp`.
- **Naming and layout** — One primary type or namespace per header. Group related declarations; keep order consistent (e.g. types, then functions, then templates).

Directory layout examples (choose one and keep it consistent):

- Flat: `include/foo.hpp`, `src/foo.cpp`
- By feature: `include/bar/foo.hpp`, `src/bar/foo.cpp`
- By layer: `include/core/`, `include/io/`, `src/core/`, `src/io/`

## Dependencies and Layering

- **No cycles** — Module A may depend on B only if B does not depend on A (directly or transitively). Break cycles by extracting a third module or inverting the dependency.
- **Explicit boundaries** — Clear “core” vs “application” vs “I/O” (or similar) so dependency direction is obvious. Document or enforce with include paths / namespaces.
- **Third-party** — Isolate external libraries behind your own headers or wrappers where beneficial, so upgrades and replacements stay local.

When adding a dependency, ask: “Does this module really need to know about that one?” Prefer passing data or callbacks over pulling in large headers.

## DRY and Reuse

- **Shared logic** — Extract to a common header (if inline/template) or to a single `.cpp` with a single declaration in a header. Do not copy-paste the same logic across files.
- **Inline / constexpr** — Use for small, stable functions that must be in headers. Avoid huge inline bodies that blow up compile times.
- **Templates** — Keep template definitions in headers (or in a `.tpp` included by the header). Factor common non-template code into a non-template helper in a `.cpp` to avoid duplication and speed up builds.

## Review Checklist

Use when reviewing or proposing C++ structure:

- [ ] Public API is in headers; implementation details are in `.cpp` or clearly marked as internal.
- [ ] Each header has a single, clear responsibility (or a small, coherent set).
- [ ] Headers include only what they need; forward declarations used where possible.
- [ ] No circular dependencies between modules.
- [ ] Shared logic exists in one place; no duplicated logic across translation units.
- [ ] Naming and file layout are consistent with the rest of the project.
- [ ] New dependencies are justified and, if possible, isolated behind your own interfaces.

## Anti-Patterns to Avoid

| Anti-pattern | Prefer instead |
|--------------|----------------|
| One giant “utils” or “common” header | Small, focused headers by concept or layer |
| Headers that include many other headers “just in case” | Minimal includes; forward declarations |
| Circular includes (A includes B, B includes A) | Extract shared types to a third header or invert dependency |
| Copy-pasted logic in multiple `.cpp` files | Single shared implementation in one header or source |
| Implementation details in public headers | Pimpl, or move implementation to `.cpp` |
| Mixing build systems or many entry points without reason | One clear build/run story (e.g. one script or one top-level CMakeLists) |

## Invariants to Enforce

- Every module has a clear dependency direction (no cycles).
- Headers define the public contract; implementation stays behind that contract.
- Duplication is removed in favor of a single, shared definition.
- Build and dependency layout remain understandable and consistent.

## When to Apply

- User asks for “modular C++,” “clean C++ architecture,” or “how to structure C++ code.”
- Adding or refactoring C++ modules, headers, or source files.
- Reviewing dependency layout, include structure, or build organization.
- Debugging link errors, circular includes, or unclear ownership (architecture often clarifies where types and code should live).

Project-specific rules (e.g. “single entry point,” “header-only utilities,” “edge-ready”) may extend or override these guidelines; apply both this skill and any project rules.
