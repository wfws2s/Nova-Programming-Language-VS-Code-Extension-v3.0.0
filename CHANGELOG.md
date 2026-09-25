# Changelog

All notable changes to the **NOVA** programming language interpreter will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [0.3.0] - 2026-09-06

### Added
- **`break` and `continue` statements** for loop control flow.
- **Standard Library `file` module**: `file.read()`, `file.write()`, `file.append()`, `file.exists()`, `file.remove()`, `file.lines()`.
- **Standard Library `os` module**: `os.args`, `os.env()`, `os.platform`, `os.exit()`, `os.cwd()`, `os.exec()`.
- **Standard Library `json` module**: `json.parse()`, `json.stringify()`.
- **Standard Library `time` module**: `time.now()`, `time.sleep()`, `time.clock()`.
- **f-String interpolation**: `f"Hello {name}!"` syntax for inline expression embedding.
- **Multiline strings**: `"""..."""` triple-quote syntax.
- **`enum` keyword**: `enum Color RED GREEN BLUE end` with dot-access `Color.RED`.
- **List comprehension**: `[x * x for x in 1..10]` and `[x for x in list if cond]`.
- **`typeof` instance returns class name**: `typeof(Dog("x","y"))` returns `"Dog"` instead of `"Instance"`.
- **Bytecode VM skeleton**: Foundation for `src/vm/` compiler and stack-based VM (in progress).
- **REPL v2**: Multiline input buffer, in-memory history navigation.

---

## [0.2.2] - 2026-08-18

### Added
- **Native Windows GUI Installer (`nova-setup.exe`)**: Zero-dependency graphical setup wizard with PATH configuration, `.nova` file association, and desktop shortcut generation.
- **PowerShell and Batch Quick-Install Scripts** (`install.ps1`, `install.bat`) for streamlined command-line installation.
- GitHub Actions CI/CD workflows for automated multi-platform builds and automated GitHub release packaging.
- Unified root project layout with C++20 CMake build configuration.
- Comprehensive developer documentation (`docs/DEVELOPMENT.md`).

### Changed
- Standardized open-source repository layout for GitHub publishing.
- Upgraded documentation and examples structure.

---

## [0.2.1] - 2026-08-10

### Added
- Standard library modules: `random`, `math`, `string`, `list`, `dict`.
- Range iteration syntax (`start..end`) in `for` loops.
- `global` variable binding keyword.
- Structured `try ... catch err ... end` error handling with call stacks.

---

## [0.2.0] - 2026-07-25

### Added
- Object-Oriented Programming (OOP) system:
  - Class definitions (`class Name ... end`)
  - Constructors (`fn init(self, ...)`)
  - Inheritance (`extends` / `:`) with `super.init()` / `super.method()`
  - Static methods (`static fn ... end`)
- Module import system (`import "./path/file.nova" as alias`).
- Windows executable icon embedding via `.rc` resource script.

---

## [0.1.0] - 2026-06-15

### Added
- Initial core interpreter implementation with C++20.
- Lexer, AST nodes, Recursive-descent Parser, and Tree-walk Interpreter.
- Dynamic typing with First-class functions and lexical closures.
- REPL interactive mode and script file execution.
- Rich error diagnostics with source line pointers and underlines.
