# MySTL

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/library-header--only-green.svg)
![Tests](https://img.shields.io/badge/tests-GoogleTest-brightgreen.svg)

A from-scratch, header-only reimplementation of the core of the C++ Standard Library — containers, iterators, algorithms, memory utilities, smart pointers, and template metaprogramming — written in modern C++.

Every data structure is implemented from the ground up (no wrapping of `std`), following the architecture of a real standard library: policy-based associative containers built on shared backbones, allocator-aware storage, exception-safe resource management, and STL-compatible iterators.

> **Goal.** Not to replace `std`, but to demonstrate deep, hands-on understanding of generic programming, data structures, memory management, template metaprogramming, exception safety, and software architecture.

A detailed engineering plan for the project lives in [`ROADMAP.md`](ROADMAP.md).

---

## Highlights

The parts worth reading first:

- **Two shared backbones.** Ordered containers (`Set`/`MultiSet`/`Map`/`MultiMap`) share a single **Red-Black Tree** with a proper `nil` sentinel and textbook CLRS insert/erase fixups. Unordered containers share a single **separate-chaining Hash Table**. Each backbone is parameterized by a key-extraction policy (`Identity` / `Select1st`), so a map and a set are the *same* tree with different policies.
- **Thread-safe `shared_ptr`.** Atomic reference counting with a polymorphic control block, a correct `weak_ptr::lock()` implemented as a lock-free CAS loop, and `make_shared` with a single combined allocation.
- **Empty-Base-Optimized `unique_ptr`.** A SFINAE-selected `compressed_ptr` gives zero storage overhead for empty deleters, with a full array (`T[]`) specialization.
- **Strong exception guarantee where it counts.** `Vector` reallocation and copy-assignment build into fresh storage and commit only on success, transferring via `move_if_noexcept` so a throwing element type never corrupts the container. This is covered by dedicated adversarial tests.
- **Small String Optimization.** `String` stores up to 15 characters inline (no heap allocation) via a union, transparently promoting to heap storage on growth.
- **Allocator-aware throughout.** A custom `allocator_traits` with full SFINAE member-detection, `rebind`, and `[[no_unique_address]]` empty-allocator storage.

---

## Features

### Sequence containers
`Vector` · `String` (SSO) · `Deque` (segmented block storage) · `List` (circular doubly-linked, sentinel) · `ForwardList` (singly-linked)

### Ordered associative containers — shared Red-Black Tree backbone
`Set` · `MultiSet` · `Map` · `MultiMap`

### Unordered associative containers — shared Hash Table backbone (separate chaining)
`UnorderedSet` · `UnorderedMultiSet` · `UnorderedMap` · `UnorderedMultiMap`

### Container adaptors
`Stack` · `Queue` · `PriorityQueue` (binary heap)

### Algorithms
`for_each` · `find` / `find_if` · `all_of` / `any_of` / `none_of` · `count` · `equal` · `lexicographical_compare` · `copy` / `move` (with `memmove` fast path for trivial types) · `fill` · `transform` · `min` / `max` / `clamp` · heap algorithms (`make_heap`, `push_heap`, `pop_heap`)

### Memory utilities
`Allocator` · `allocator_traits` · `construct_at` / `destroy_at` / `destroy` · `uninitialized_copy` / `uninitialized_move` / `uninitialized_move_if_noexcept` / `uninitialized_fill` (with RAII rollback guards)

### Smart pointers
`unique_ptr` (+ `T[]`) · `shared_ptr` · `weak_ptr` · `make_unique` · `make_shared`

### Utilities & metaprogramming
`Pair` · `Tuple` (with `get` / `tuple_size` / `tuple_element` / structured bindings) · `type_traits` · `iterator_traits` · `reverse_iterator` · transparent comparators · `reference_wrapper` · FNV-1a hash framework

---

## Architecture

The library is organized as a layered dependency DAG — no circular dependencies.

```
                     +-----------------------------+
                     |          Adaptors           |
                     | Stack   Queue   PriorityQ   |
                     +--------------+--------------+
                                    |
        +---------------------------+---------------------------+
        |                          |                            |
+-------v--------+       +---------v----------+       +---------v---------+
| Ordered assoc. |       | Unordered assoc.   |       | Sequence          |
| Set/Map/Multi* |       | Unordered{Set,Map} |       | Vector/String/... |
+-------+--------+       +---------+----------+       +-------------------+
        |                          |
+-------v--------+       +---------v----------+
| Red-Black Tree |       |     Hash Table     |
+-------+--------+       +---------+----------+
        |                          |
+-------v--------------------------v-----------------------------+
|   utility  |  memory  |  functional  |  algorithm             |
+-------+--------------------------------------------------------+
        |
+-------v--------------------------------------------------------+
|   type_traits   |   iterator   |   allocator   |   cstddef     |
+----------------------------------------------------------------+
```

Both associative families are built on a single shared backbone each, selected by a key-extraction policy:

```cpp
// A Map and a Set are the same tree with a different policy:
Map<K, V> ── RBTree<K, Pair<const K,V>, Select1st, Compare, Alloc>
Set<K>    ── RBTree<K, K,               Identity,  Compare, Alloc>
```

---

## Design principles

- Header-only, modern C++
- Policy-based design (shared RB-Tree / Hash-Table backbones)
- Generic programming with SFINAE-constrained templates
- STL-compatible iterators (const/non-const conversion, tag dispatch)
- Allocator awareness via a custom `allocator_traits`
- Empty Base Optimization (`[[no_unique_address]]`, `compressed_ptr`)
- Exception-aware resource management (RAII rollback guards, copy-and-swap)
- Zero circular dependencies

---

## Container reference

| Container | Underlying structure | Iterator category |
|---|---|---|
| `Vector` | dynamic contiguous array | random access |
| `String` | Small String Optimization (SSO, 15 chars inline) | random access |
| `Deque` | map of fixed-size blocks | random access |
| `List` | circular doubly-linked list + sentinel | bidirectional |
| `ForwardList` | singly-linked list + head sentinel | forward |
| `Set` / `MultiSet` | Red-Black Tree | bidirectional (const) |
| `Map` / `MultiMap` | Red-Black Tree | bidirectional |
| `UnorderedSet` / `UnorderedMultiSet` | separate-chaining hash table | forward |
| `UnorderedMap` / `UnorderedMultiMap` | separate-chaining hash table | forward |
| `Stack` | adaptor over `Deque` | — |
| `Queue` | adaptor over `Deque` | — |
| `PriorityQueue` | binary heap over `Vector` | — |

---

## Exception safety

Guarantees are documented per operation and verified with instrumented, throw-on-demand test types:

- **Strong guarantee:** `Vector` reallocation, copy-assignment, and copy/fill construction — the container is unchanged if any element operation throws (validated in [`tests/test_vector.cpp`](tests/test_vector.cpp)).
- **Basic guarantee with rollback:** node-based containers (`List`, `ForwardList`, `Deque`, RB-Tree, Hash-Table) release partially-constructed nodes/blocks on failure — no leaks, no double frees.
- **Smart pointers:** correct reference-count teardown and object/control-block lifetime under copy, move, reset, and `weak_ptr` expiry.

---

## Building & testing

**Requirements:** a C++20 compiler, CMake ≥ 3.14. GoogleTest is fetched automatically via `FetchContent`.

```bash
git clone https://github.com/HenrikGharagyozyan/MySTL.git
cd MySTL

cmake -B build
cmake --build build

ctest --test-dir build --output-on-failure
```

Each component has an independent GoogleTest executable under [`tests/`](tests/) (containers, iterators, algorithms, memory utilities, smart pointers, RB-Tree, Hash-Table).

### Using it in your own project

MySTL is a header-only `INTERFACE` CMake target:

```cmake
add_subdirectory(MySTL)
target_link_libraries(your_target PRIVATE mystl)
```

```cpp
#include "mystl/vector.hpp"
#include "mystl/map.hpp"

mystl::Vector<int> v = {1, 2, 3};
mystl::Map<std::string, int> m;
m["answer"] = 42;
```

---

## Project layout

```
MySTL/
├── include/mystl/     # the library (header-only)
├── tests/             # GoogleTest suites, one per component
├── CMakeLists.txt     # header-only target + GoogleTest integration
├── ROADMAP.md         # engineering roadmap
└── Readme.md
```

| Property | Value |
|---|---|
| Language | C++20 |
| Library type | Header-only |
| Headers | 32 |
| Approx. LOC | ~8,400 |
| Tests | GoogleTest (28 suites) |
| Build | CMake |

---

## Current limitations

Some areas are intentionally simplified; they are tracked in [`ROADMAP.md`](ROADMAP.md):

- Allocator **propagation traits** (POCCA / POCMA / POCS) are defined in `allocator_traits` but not yet honored by container assignment/swap, so **stateful, non-equal allocators are not fully supported**.
- `String` currently exposes a minimal API (no `append` / `substr` / `find` yet) and is not yet integrated with the hash framework.
- Exception guarantees are strongest in `Vector`; node-based containers currently provide the basic guarantee.
- The library targets C++20 but is largely written in a C++17 idiom — **Concepts, Ranges interoperability, and `operator<=>` are planned, not yet implemented**.
- Unordered containers expose a smaller API surface (e.g. no `reserve`, `bucket`, or local iterators) than their standard counterparts.

---

## Roadmap

Planned work, in priority order (full detail in [`ROADMAP.md`](ROADMAP.md)):

1. **Correctness & tooling** — CI across GCC/Clang/MSVC, ASan/UBSan/TSan, fix known UB.
2. **Full allocator propagation** — honor POCCA/POCMA/POCS, proven with a stateful arena allocator.
3. **Make C++20 real** — Concepts-constrained templates, `std::ranges` interoperability, `operator<=>`.
4. **Depth & measurement** — a complete `constexpr` SSO `String`, and a benchmark suite comparing against `libstdc++` / `libc++`.

---

## Philosophy

MySTL is an engineering-focused exploration of how a modern generic library is designed and implemented — data structures, memory management, iterator abstractions, template programming, and architecture — with an emphasis on readability, correctness, and honest documentation of its own tradeoffs.
