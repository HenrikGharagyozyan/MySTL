# MySTL

A modern C++20 implementation of the Standard Template Library (STL) built from scratch for educational purposes.

The goal of this project is to gain a deep understanding of how the C++ Standard Library works internally by implementing containers, iterators, allocators, algorithms, and memory utilities without relying on the STL implementations themselves.

---

## Features

- Header-only library
- Modern C++20
- Custom allocator and `allocator_traits`
- Generic containers
- Iterator support
- Unit tested with GoogleTest
- Built with CMake

---

## Project Structure

```
MySTL/
│
├── include/
│   └── mystl/
│       ├── algorithm.hpp
│       ├── allocator.hpp
│       ├── deque.hpp
│       ├── forward_list.hpp
│       ├── functional.hpp
│       ├── iterator.hpp
│       ├── list.hpp
│       ├── map.hpp
│       ├── memory.hpp
│       ├── priority_queue.hpp
│       ├── queue.hpp
│       ├── rb_tree.hpp
│       ├── set.hpp
│       ├── stack.hpp
│       ├── string.hpp
│       ├── type_traits.hpp
│       ├── unordered_map.hpp
│       ├── unordered_multimap.hpp
│       ├── unordered_multiset.hpp
│       ├── unordered_set.hpp
│       ├── utility.hpp
│       └── vector.hpp
│
├── tests/
│
├── benchmarks/
│
└── CMakeLists.txt
```

---

## Implemented Components

### Sequence Containers

- ✅ Vector
- ✅ List
- ✅ Forward List
- ✅ Deque

### Associative Containers

- ✅ Set
- ✅ Map

### Unordered Containers

- ✅ Unordered Set
- ✅ Unordered MultiSet
- ✅ Unordered Map
- ✅ Unordered MultiMap

### Container Adaptors

- ✅ Stack
- ✅ Queue
- ✅ Priority Queue

### Core Components

- ✅ Red-Black Tree
- ✅ Iterator Framework
- ✅ Algorithms
- ✅ Memory Utilities
- ✅ Allocators
- ✅ Functional Objects
- ✅ Utility Library
- ✅ Type Traits

---

## Example

```cpp
#include <mystl/vector.hpp>

int main()
{
    mystl::Vector<int> numbers;

    numbers.push_back(10);
    numbers.push_back(20);
    numbers.push_back(30);

    for (auto value : numbers)
    {
        std::cout << value << '\n';
    }
}
```

---

## Building

Clone the repository

```bash
git clone https://github.com/<your-username>/MySTL.git
cd MySTL
```

Configure

```bash
cmake -B build
```

Build

```bash
cmake --build build
```

Run all tests

```bash
ctest --test-dir build --output-on-failure
```

---

## Testing

The project uses **GoogleTest** for unit testing.

GoogleTest is automatically downloaded using **CMake FetchContent**, so no manual installation is required.

Current test coverage includes:

- Vector
- List
- Forward List
- Deque
- Stack
- Queue
- Priority Queue
- Set
- Map
- Unordered Containers
- Algorithms

---

## Design Goals

- Follow the behavior of the C++ Standard Library
- Generic and reusable implementation
- Strong exception safety where applicable
- Modern C++20 style
- Extensive unit testing
- Educational implementation without copying STL sources

---

## Roadmap

### Containers

- Improve `basic_string`
- Small Buffer Optimization (SBO)
- Additional allocator-aware constructors

### Algorithms

- Sorting algorithms
- Heap algorithms
- Numeric algorithms
- Range algorithms

### Performance

- Optimized move semantics
- Better exception guarantees
- Benchmark suite

---

## Technologies

- C++20
- CMake
- GoogleTest

---

## License

This project is released under the MIT License.