# Oxidize

Oxidize is a high-performance, safe, and expressive C++ library inspired by the Rust standard library. It reimagines common C++ standard library components and concurrency primitives through the lens of Rust’s ownership and safety guarantees — helping developers write robust, maintainable, and efficient applications.

## Why Oxidize?

- **Safety first:** Avoid common C++ bugs like data races, use-after-free, and subtle concurrency errors by using Rust-inspired patterns.
- **Expressive APIs:** Leverage modern C++ features to get ergonomics and clarity reminiscent of Rust.
- **High performance:** Zero-cost abstractions and minimal runtime overhead.
- **Productivity:** Simplify complex concurrency and error handling code with easy-to-use abstractions.
- **Open and extensible:** Oxidize is fully open source and designed to welcome community contributions.

## Features

- Ownership-inspired types: `Result`, `Option`, `Box`, `Arc`, etc.
- Concurrency primitives: `Mutex` with poisoning, `MutexGuard` for RAII locking.
- Safe error handling patterns with `panic`.
- Move semantics and smart resource management.

## Basic Usage
```cpp
#include "oxidize/prelude.hpp"
using namespace ox;

int main() {
    Option<i32> opt = Some(10);
    auto res = opt.ok_or(String("Option is None!"));

    if(res.is_ok()) {
        println("Value: {}", res.unwrap());
    }else {
        println("Error: {}", res.unwrap_err());
    }

    return 0;
}
```

## Contributing

Contributions are welcome! Whether you want to report bugs, suggest features, or submit pull requests, please follow these guidelines:

* Follow the existing code style and naming conventions.
* Write clear, concise commit messages.
* Include tests for new functionality.
* Participate in code reviews.

Check out the [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

## License

Oxidize is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Contact

Created by Lucas Oliveira. Feel free to reach out via GitHub issues or email: [lucas.barros1804@gmail.com](mailto:lucas.barros1804@gmail.com)