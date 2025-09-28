# Standard Library Data Structure Implementations

Custom implementations of C++ standard library data structures, built from scratch to demonstrate low-level programming concepts and modern C++ techniques.

## Overview

This repository contains header-only implementations of fundamental C++ containers, designed with:

- **Performance**: Efficient memory management and optimized algorithms
- **Modern C++**: Template metaprogramming, perfect forwarding, and RAII
- **STL Compliance**: Drop-in replacements with full iterator support
- **Educational Value**: Clean, readable code demonstrating best practices

## Data Structures

### Currently Implemented

- **`stack/`** - Dynamic stack with random access iterators
- **`vector/`** - Dynamic array with contiguous memory layout  
- **`deque/`** - Double-ended queue with efficient front/back operations

Each implementation includes:
- Full iterator support (forward, reverse, const variants)
- Exception safety guarantees
- Custom allocator support
- STL-compatible interface

## Usage

Since these are header-only implementations, simply include the desired header:

```cpp
#include "stack/stack.h"
#include "vector/vector.h" 
#include "deque/deque.h"

int main() {
    // Use exactly like standard library containers
    stack<int> s{1, 2, 3, 4, 5};
    vector<std::string> v{"hello", "world"};
    deque<double> d;
    
    // Full STL compatibility
    s.push(6);
    v.emplace_back("!");
    d.push_front(3.14);
    
    // Iterator support
    for (const auto& item : s) {
        std::cout << item << " ";
    }
    
    return 0;
}
```

## Features

- **Header-Only**: No compilation required, just include and use
- **STL Compatible**: Works with all standard algorithms and range-based loops
- **Exception Safe**: Strong exception safety guarantees with RAII
- **Modern C++**: C++17 features, perfect forwarding, move semantics
- **Custom Allocators**: Support for custom memory allocation strategies

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## Project Structure

```
├── stack/
│   └── stack.h
├── vector/  
│   └── vector.h
├── deque/
│   └── deque.h
└── README.md
```

## Design Goals

These implementations prioritize:

1. **Educational Clarity**: Code is written to be readable and instructive
2. **Performance**: Efficient algorithms and memory management
3. **Correctness**: Thorough handling of edge cases and exceptions
4. **Compatibility**: Drop-in replacements for standard containers

## License

MIT License - feel free to use and modify as needed.