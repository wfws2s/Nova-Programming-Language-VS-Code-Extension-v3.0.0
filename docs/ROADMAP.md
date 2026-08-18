# NOVA — Language & Architecture Roadmap

> **Status:** Active Roadmap & Release Milestone Tracking.

---

## Completed Milestones

| Area / Milestone | Version | Status |
|---|---|---|
| **Core Architecture**: C++20 engine, Pratt expression parser, AST nodes, Lexer with precise diagnostics | v0.1.0 | :white_check_mark: Complete |
| **Interactive REPL & CLI**: File execution, command-line arguments, line evaluation | v0.1.0 | :white_check_mark: Complete |
| **Data Types**: Numbers, Strings, Booleans, Lists, Dicts, Functions, Nil | v0.1.0 | :white_check_mark: Complete |
| **Control Flow**: `if/elif/else`, `while`, `for in`, ranges `a..b`, `try/catch` | v0.2.0 | :white_check_mark: Complete |
| **Standard Modules**: `math`, `string`, `list`, `dict`, `random` | v0.2.1 | :white_check_mark: Complete |
| **Object-Oriented Programming (OOP)**: `class`, `extends`, `init`, `self`, `super`, `static fn` | v0.2.2 | :white_check_mark: Complete |
| **Module & File Imports**: Direct path imports (`import "./file.nova" as alias`) | v0.2.2 | :white_check_mark: Complete |
| **Distribution & Installation**: Native Windows GUI Setup Wizard (`nova-setup.exe`), PATH updater, file associations | v0.2.2 | :white_check_mark: Complete |
| **CI/CD & Open Source**: GitHub Actions multi-platform workflows, templates, and contributor guides | v0.2.2 | :white_check_mark: Complete |

---

## Upcoming Milestones

### Milestone 1: Standard Library Expansion (v0.3.0)
- [ ] **`file` / `io` module**: `file.read(path)`, `file.write(path, data)`, `file.exists(path)`, `file.remove(path)`.
- [ ] **`os` / `sys` module**: `os.args`, `os.env(name)`, `os.exit(code)`, `os.exec(cmd)`.
- [ ] **`json` module**: `json.parse(str)`, `json.stringify(val)`.
- [ ] **`time` module**: `time.now()`, `time.sleep(ms)`, `time.format()`.

### Milestone 2: Tooling & Developer Experience (v0.3.5)
- [ ] **VS Code Extension**: TextMate grammar for `.nova` syntax highlighting.
- [ ] **Interactive REPL v2**: Syntax colorization, multi-line buffer editing, history persistence (`~/.nova_history`).
- [ ] **Language Server Protocol (LSP)**: Diagnostics, hover type info, and autocompletion.

### Milestone 3: Bytecode Virtual Machine & Performance (v0.4.0)
- [ ] **Bytecode Compiler**: Transform AST into flat chunk bytecode instructions (`OP_LOAD_CONST`, `OP_ADD`, `OP_CALL`).
- [ ] **Stack-based VM**: Fast execution loop avoiding recursive C++ function stack overhead.
- [ ] **Garbage Collector**: Tracing mark-and-sweep or generational GC for complex cycle graphs.

### Milestone 4: Package Manager & Ecosystem (v1.0.0)
- [ ] **Nova Package Manager (`novapm` / `nova pkg`)**: Dependency resolution and package installation from GitHub.
- [ ] **C / C++ Foreign Function Interface (FFI)**: Load dynamic libraries (`.dll`, `.so`) directly in Nova code.
