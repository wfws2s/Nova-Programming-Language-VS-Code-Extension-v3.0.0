# Contributing to Nova

Thank you for your interest in contributing to the **NOVA** programming language! Nova is an open-source project, and we welcome contributions of all kinds: bug reports, feature suggestions, documentation improvements, standard library modules, and performance optimizations.

---

## Code of Conduct

We are committed to providing a welcoming, inclusive, and harassment-free experience for everyone. Please be respectful, constructive, and helpful in all interactions.

---

## How Can You Contribute?

1. **Reporting Bugs**: Check existing issues first. If not reported, submit a [Bug Report](https://github.com/nova-lang/nova/issues/new?template=bug_report.md) with a minimal code example reproducing the issue.
2. **Suggesting Features**: Open a [Feature Request](https://github.com/nova-lang/nova/issues/new?template=feature_request.md) with proposed syntax and use cases.
3. **Improving Documentation**: Fix typos, add tutorials, or expand on language syntax in the `docs/` folder.
4. **Submitting Code**: Fix an open issue or implement an approved feature.

---

## Getting Started with Development

### Prerequisites
- **C++20 Compiler**: GCC 13+, Clang 17+, or MSVC 2022 (v143+).
- **CMake**: Version 3.20 or newer.
- **Build Generator**: [Ninja](https://ninja-build.org/) (recommended) or Make.

### Fork & Clone
```bash
git clone https://github.com/<your-username>/nova.git
cd nova
```

### Build from Source
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Run the Test Suite
Always ensure all tests pass before submitting a pull request:
```bash
ctest --test-dir build --output-on-failure
```

---

## Project Architecture & Structure

```
nova/
├── src/
│   ├── lexer/          # Token definitions, Scanner/Tokenizer, Lexer error tracking
│   ├── ast/            # Abstract Syntax Tree node hierarchies
│   ├── parser/         # Recursive descent parser & syntax error reporting
│   ├── runtime/        # Value types, Environments, Scopes, Built-in functions & modules
│   ├── interpreter/    # Tree-walk AST evaluator & execution engine
│   └── main.cpp        # CLI entry point, REPL, and argument handler
├── tests/              # CTest test suite (smoke, lexer, parser, interpreter)
├── installer/          # Native Win32 GUI Installer (nova-setup.exe)
├── examples/           # Standard Nova code examples & feature demos
├── docs/               # Language specification, grammar, development guides
└── .github/            # GitHub Actions CI/CD workflows and issue templates
```

For a comprehensive guide on adding new keywords, AST nodes, or built-in functions, see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

---

## Coding Guidelines

- **C++ Standard**: C++20 (`std::string_view`, `std::variant`, `std::optional`, `std::unique_ptr`, `constexpr`, etc.).
- **Code Style**:
  - `camelCase` for variables and function names.
  - `PascalCase` for classes, structs, and enums.
  - 4 spaces indentation (no tabs).
  - Explicit error handling with meaningful messages and diagnostics.
- **Memory Safety**: Prefer smart pointers (`std::shared_ptr`, `std::unique_ptr`) and RAII over raw pointer manual allocation.

---

## Pull Request Process

1. **Branch Naming**:
   - `feature/your-feature-name`
   - `fix/issue-description`
   - `docs/what-changed`
2. **Commit Messages**: Follow Conventional Commits format:
   - `feat: add math.hypot function`
   - `fix: handle unary minus on negative floats`
   - `docs: update OOP inheritance examples`
3. **Tests**: Add unit tests in `tests/` for any new language features or bug fixes.
4. **Open PR**: Fill out the [Pull Request Template](.github/pull_request_template.md) and link any related issues.
