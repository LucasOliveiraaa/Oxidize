# Contributing to Oxidize

Thank you for your interest in contributing to **Oxidize**! This project aims to bring safe, expressive, and high-performance Rust-like patterns to modern C++, and we welcome help to grow and improve it.

Whether you're fixing bugs, improving documentation, implementing new features, or refining the API, your contributions are appreciated.

---

## Getting Started

1. **Fork the repository** and clone it locally.
2. Create a new branch from `development`:
   ```bash
    git checkout development
    git checkout -b feature/your-feature-name
    ```
3. Make your changes with clear, well-structured commits.
4. Write tests or examples for any new functionality.
5. Ensure the project builds and all tests pass.
6. Open a pull request to the `development` branch.

---

## Branching Strategy

* **`main`**: stable, production-ready code only.
* **`development`**: active development and testing.
* **`feature/*`**: topic branches for new features, fixes, or refactors (merged into `development`).

Once changes in `development` are tested and stable, they are merged into `main`.

---

## Code Guidelines

* Follow modern C++ best practices (C++17 and up).
* Avoid raw pointers unless absolutely necessary.
* Prefer `ox::Result`, `ox::Option`, and RAII-style constructs.
* Minimize unsafe operations; encapsulate them where necessary.
* Use `move` semantics and `constexpr` wherever beneficial.

---

## Tests

* Add or update unit tests for any new features or bug fixes.
* Tests should live in the `/tests/` directory.
* Use clear, descriptive test names and keep tests isolated.

---

## Documentation

* Document public APIs and modules clearly.
* Add example usage in `/examples/` when helpful.
* Prefer in-code comments over external documentation when possible.

---

## Code of Conduct

Be respectful and constructive in all discussions and code reviews. We value inclusivity, collaboration, and learning. Disrespectful behavior will not be tolerated.

---

## Tools & CI

We use GitHub Actions for continuous integration. All pull requests must pass CI checks before being merged.

---

## Questions?

If you're unsure about anything or want to discuss an idea before implementing it, feel free to:

* Open a GitHub issue
* Start a discussion in the [Discussions](https://github.com/LucasOliveiraaa/Oxidize/discussions) tab
* Reach out directly to the maintainer
