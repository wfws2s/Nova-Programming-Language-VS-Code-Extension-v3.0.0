<div align="center">

# 🌟 NOVA Programming Language

**A modern, readable, dynamically-typed programming language engineered with C++20 performance.**

[![CI](https://github.com/nova-lang/nova/actions/workflows/ci.yml/badge.svg)](https://github.com/nova-lang/nova/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-0.2.2-blue.svg)](https://github.com/nova-lang/nova/releases)
[![C++20](https://img.shields.io/badge/standard-C%2B%2B20-crimson.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://github.com/nova-lang/nova)

<p align="center">
  <b>Easy to write. Flexible to use. Safe to run. Predictable to understand.</b>
</p>

[Quick Start](#-quick-start) •
[Installation](#-installation) •
[Features](#-key-features) •
[Documentation](#-documentation) •
[Contributing](#-contributing) •
[Roadmap](#-roadmap)

</div>

---

## 📖 About Nova

**NOVA** is a modern scripting and general-purpose programming language that combines **Python's readability**, **JavaScript's dynamic flexibility**, and **Lua's clean embeddable simplicity**, driven by a blazing-fast **C++20 tree-walk runtime engine**.

Whether you are building tools, game scripting engines, automation workflows, or learning language design, Nova offers a comfortable syntax without the boilerplate.

---

## ✨ Key Features

- 💎 **Clean Block Syntax**: Readable code structured with `end` blocks (`if ... end`, `fn ... end`, `class ... end`, `for ... end`).
- 🏛️ **Full Object-Oriented Programming (OOP)**:
  - Classes, constructors (`fn init(self, ...)`), instance variables, and methods.
  - Single inheritance (`class Dog extends Animal` or `:`), parent calls via `super.init(...)` and `super.method(...)`.
  - Static methods (`static fn ... end`) and property access.
- 📦 **Direct Module & File Imports**: Import local files and modules directly with aliases (`import "./helper.nova" as helper`).
- ⚡ **Standard Library Built-ins**:
  - `random`: `randint`, `choice`, `shuffle`, `uniform`, `range`
  - `math`: `sqrt`, `pow`, `abs`, `floor`, `ceil`, `round`, `sin`, `cos`, `tan`, `pi`, `e`
  - `string`: `to_upper`, `to_lower`, `trim`, `starts_with`, `ends_with`, `contains`, `join`
  - `list`: `contains`, `index_of`, `reverse`, slicing `list[start:end]`
  - `dict`: `has_key`, `keys`, `values`
- 🛡️ **Structured Error Handling**: `try ... catch err ... end` with stack traces and rich diagnostics displaying source pointers.
- 🔍 **Type Introspection**: `type(x)`, `typeof(x)`, and `x is "Type"` operators.
- 🛠️ **Cross-Platform**: First-class support for Windows, Linux, and macOS.

---

## 🚀 Code Preview

```nova
import random as rnd
import math as m

# 1. Object-Oriented Programming
class Shape
    fn init(self, name)
        self.name = name
    end

    fn area(self)
        return 0
    end

    static fn category()
        return "2D Geometric Shapes"
    end
end

class Circle extends Shape
    fn init(self, radius)
        super.init("Circle")
        self.radius = radius
    end

    fn area(self)
        return m.pi * self.radius * self.radius
    end
end

# 2. Instantiation & Polymorphism
let c = Circle(5.0)
print(c.name + " area: " + string(c.area()))
print("Category: " + Shape.category())

# 3. Lists, Ranges & Standard Library
let numbers = [10, 20, 30, 40, 50]
print("Random choice:", rnd.choice(numbers))

for i in 1..5
    print("Loop step:", i)
end
```

---

## 📦 Installation

### Method 1: Standalone Windows GUI Setup Wizard (`Install.exe`) — (Recommended)

1. Download or double-click **`Install.exe`** (Single self-contained file with embedded Nova interpreter & VS Code extension).
2. Follow the setup wizard:
   - **Step 1: Welcome** — Overview of Nova language features.
   - **Step 2: License Agreement** — Read and accept terms.
   - **Step 3: Destination & Options** — Choose folder (default: `%LOCALAPPDATA%\Programs\Nova`), auto-configure **PATH**, and associate `.nova` files.
   - **Step 4: Ready to Install** — Review settings and click **Install**.
   - **Step 5: Finish** — Check options to launch Nova Terminal immediately or open install folder.

> [!TIP]
> **Zero Dependencies**: `Install.exe` does NOT require CMake, compilers, or any downloads. It installs everything in seconds!

### Method 2: Installing the VS Code Extension (`.vsix`)

After running `Install.exe`, the extension `nova-lang-0.4.0.vsix` is automatically placed in your Nova installation directory:
1. Open **Visual Studio Code**.
2. Press `Ctrl + Shift + P` (or `Cmd + Shift + P` on macOS) to open the Command Palette.
3. Type and select **`Extensions: Install from VSIX...`**.
4. Navigate to your Nova folder and select **`nova-lang-0.4.0.vsix`**.
5. Enjoy full syntax highlighting, bracket matching, and code snippets for `.nova` files!

### Method 3: PowerShell Quick Install (Windows)

Open PowerShell and run:
```powershell
.\install.ps1
```

### Method 4: Build from Source (Windows, Linux, macOS)

#### Prerequisites
- **CMake 3.20+**
- **C++20 Compiler** (GCC 13+, Clang 17+, or MSVC 2022)
- **Ninja** (Recommended) or Make

#### Build Commands
```bash
# Clone the repository
git clone https://github.com/nova-lang/nova.git
cd nova

# Configure CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build interpreter & test suite
cmake --build build

# Run automated tests
ctest --test-dir build --output-on-failure
```

The compiled binary `nova` (or `nova.exe` on Windows) is placed in `build/`.

---

## 💻 Usage

### Run a Script
```bash
nova path/to/script.nova
```

### Interactive REPL Mode
Start interactive mode by simply running `nova` without arguments:
```bash
$ nova
Nova Programming Language v0.2.2
Type 'exit' to quit.
>>> let x = 42
>>> print("Hello from Nova! x is:", x)
Hello from Nova! x is: 42
>>> 
```

### Check Version
```bash
nova --version
```

---

## 📚 Documentation

Detailed specifications and language guides are located in the [`docs/`](docs/) directory:

- 📘 [**Language Overview**](docs/LANGUAGE.md) — Design principles, memory model, and execution semantics.
- 📙 [**Syntax Specification**](docs/SYNTAX.md) — Comprehensive syntax cheat-sheet and control flow reference.
- 📗 [**Type System**](docs/TYPES.md) — Built-in data types, conversions, and OOP object models.
- 📕 [**Error Handling**](docs/ERRORS.md) — Diagnostics, exception rules, and traceback format.
- 🛠️ [**Developer & Extension Guide**](docs/DEVELOPMENT.md) — Step-by-step tutorial on adding new syntax, built-ins, and upgrading versions.
- 🗺️ [**Roadmap**](docs/ROADMAP.md) — Upcoming releases, Bytecode VM architecture, and ecosystem tools.

---

## 🧪 Testing

Nova is backed by an automated regression test suite:

```bash
ctest --test-dir build --output-on-failure
```

Tests include:
- `smoke`: Sanity & compilation validation.
- `lexer`: Token stream parsing and diagnostics.
- `parser`: AST generation and syntax validation.
- `interpreter`: OOP, closures, module imports, control flow, builtins.

---

## 🤝 Contributing

We welcome contributions from everyone! Please read our [**Contributing Guide**](CONTRIBUTING.md) to get started with:
- Submitting bug reports & feature requests.
- Writing unit tests and code standard guidelines.
- Submitting pull requests.

---

## 📄 License

Nova is released under the **[MIT License](LICENSE)**. You are free to use, modify, distribute, and embed Nova in private or commercial software.

---

<div align="center">
  <sub>Developed with ❤️ for the open-source community.</sub>
</div>
