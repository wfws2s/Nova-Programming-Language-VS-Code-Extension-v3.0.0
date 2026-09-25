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
| **Standard Library Expansion**: `file`/`io`, `os`/`sys`, `json`, `time` modules | v0.3.0 | :white_check_mark: Complete |
| **Control Flow & Syntax Enhancement**: `break`/`continue`, `enum`, list comprehension, f-strings `f"..."`, multiline strings `"""..."""` | v0.3.0 | :white_check_mark: Complete |
| **REPL v2 (Multi-line Input)**: Multi-line buffer editing and CLI args | v0.3.0 | :white_check_mark: Complete |
| **Bytecode VM Skeleton**: OpCodes, Chunk bytecode emitter, AST Compiler, and Stack-based VM engine (`src/vm/`) | v0.3.0 | :white_check_mark: Complete |

---

## Upcoming Milestones

### Milestone 2: Tooling & Developer Experience (v0.3.5)
- [x] **f-String and Multiline String Syntax**: Integrated in language parser.
- [x] **Interactive REPL v2 (Multi-line)**: Block-depth tracking and statement buffering.
- [ ] **Language Server Protocol (LSP)**: Diagnostics, hover type info, and autocompletion.
- [ ] **REPL History Persistence**: History persistence (`~/.nova_history`).

### Milestone 3: Bytecode Virtual Machine & Performance (v0.4.0)
- [x] **Bytecode Foundation**: Chunk bytecode emitter, constant pool, OpCodes, and stack execution loop.
- [ ] **Bytecode Compiler Expansion**: Full AST coverage (closures, OOP supercalls, exception frames) into bytecode.
- [ ] **Garbage Collector**: Tracing mark-and-sweep or generational GC for complex cycle graphs.

### Milestone 4: Package Manager & Ecosystem (v1.0.0)
- [ ] **Nova Package Manager (`novapm` / `nova pkg`)**: Dependency resolution and package installation from GitHub.
- [ ] **C / C++ Foreign Function Interface (FFI)**: Load dynamic libraries (`.dll`, `.so`) directly in Nova code.
