# Nova Development & Extension Guide

Welcome to the internal engineering guide for the **NOVA** programming language interpreter. This document provides step-by-step instructions on the internal architecture, how to extend the language, and how to maintain, test, and release new versions.

---

## Table of Contents
1. [Architecture Overview](#1-architecture-overview)
2. [Adding a New Syntax / Keyword](#2-adding-a-new-syntax--keyword)
3. [Adding Built-in Functions & Standard Modules](#3-adding-built-in-functions--standard-modules)
4. [Writing & Running Tests](#4-writing--running-tests)
5. [Version Upgrade & Release Checklist](#5-version-upgrade--release-checklist)
6. [Future Development Roadmap](#6-future-development-roadmap)

---

## 1. Architecture Overview

Nova uses a clean, multi-stage interpreter pipeline written in modern C++20:

```
+-------------+      +-------------+      +-------------+      +------------------+
| Source Code | ---> |    Lexer    | ---> |   Parser    | ---> |   Interpreter    |
|   (.nova)   |      | (Tokenizer) |      | (AST Tree)  |      | (Tree-Walk / Env)|
+-------------+      +-------------+      +-------------+      +------------------+
                            |                    |                      |
                            v                    v                      v
                      Token Stream           AST Nodes             Runtime Values
```

### Key Source Folders (`src/`)

| Folder | Responsibility | Main Files |
| --- | --- | --- |
| `src/lexer/` | Converts raw text into a sequence of typed `Token` objects. Tracks line/column numbers for diagnostics. | `token.hpp`, `token.cpp`, `lexer.hpp`, `lexer.cpp` |
| `src/ast/` | Defines Abstract Syntax Tree structures (Expressions & Statements) using modern C++ polymorphism. | `ast.hpp`, `ast.cpp` |
| `src/parser/` | Recursive-descent parser that consumes tokens and produces AST nodes. Performs operator precedence parsing. | `parser.hpp`, `parser.cpp` |
| `src/runtime/` | Represents runtime data structures (`Value` variant, `Environment` symbol tables, lexical scopes, closures, OOP classes/instances). | `value.hpp`, `value.cpp`, `environment.hpp`, `builtins.hpp`, `builtins.cpp` |
| `src/interpreter/` | Evaluates AST nodes by traversing the tree, managing control flow (`return`, `break`, `continue`, exceptions), and calling built-ins. | `interpreter.hpp`, `interpreter.cpp`, `diagnostics.hpp`, `diagnostics.cpp` |

---

## 2. Adding a New Syntax / Keyword

Suppose you want to add a new keyword or statement (for example, a `repeat N times ... end` loop or a new binary operator):

### Step 1: Register Token Type
Edit `src/lexer/token.hpp` and `src/lexer/token.cpp`:
1. Add the enum entry in `TokenType`:
   ```cpp
   enum class TokenType {
       // ...
       REPEAT,
       TIMES,
       // ...
   };
   ```
2. Add the keyword string mapping in `src/lexer/lexer.cpp`:
   ```cpp
   const std::unordered_map<std::string, TokenType> keywords = {
       // ...
       {"repeat", TokenType::REPEAT},
       {"times", TokenType::TIMES},
   };
   ```

### Step 2: Define AST Node
Edit `src/ast/ast.hpp`:
```cpp
class RepeatStmt : public Statement {
public:
    std::unique_ptr<Expression> countExpr;
    std::vector<std::unique_ptr<Statement>> body;

    RepeatStmt(std::unique_ptr<Expression> count, std::vector<std::unique_ptr<Statement>> bodyStatements, SourceLocation loc)
        : countExpr(std::move(count)), body(std::move(bodyStatements)) {
        this->location = loc;
    }

    void accept(ASTVisitor& visitor) override;
};
```

### Step 3: Parse the Syntax in Parser
Edit `src/parser/parser.cpp`:
1. In `parseStatement()`, detect `TokenType::REPEAT`:
   ```cpp
   if (match(TokenType::REPEAT)) {
       return parseRepeatStatement();
   }
   ```
2. Implement `parseRepeatStatement()`:
   ```cpp
   std::unique_ptr<Statement> Parser::parseRepeatStatement() {
       SourceLocation loc = previous().location;
       auto count = parseExpression();
       consume(TokenType::TIMES, "Expected 'times' after repeat count expression.");
       
       std::vector<std::unique_ptr<Statement>> body;
       while (!check(TokenType::END) && !isAtEnd()) {
           body.push_back(parseStatement());
       }
       consume(TokenType::END, "Expected 'end' to close repeat block.");
       return std::make_unique<RepeatStmt>(std::move(count), std::move(body), loc);
   }
   ```

### Step 4: Evaluate in Interpreter
Edit `src/interpreter/interpreter.cpp`:
1. Implement the AST visitor method:
   ```cpp
   void Interpreter::visit(RepeatStmt& stmt) {
       Value countVal = evaluate(*stmt.countExpr);
       if (!countVal.isNumber()) {
           throw RuntimeError("Repeat count must be a number", stmt.location);
       }
       int64_t times = static_cast<int64_t>(countVal.asNumber());
       for (int64_t i = 0; i < times; ++i) {
           for (const auto& s : stmt.body) {
               execute(*s);
           }
       }
   }
   ```

---

## 3. Adding Built-in Functions & Standard Modules

Nova standard library modules (like `math`, `random`, `string`, `list`, `dict`) are registered in `src/runtime/builtins.cpp`.

### Adding a global function (e.g. `time()`)
In `src/runtime/builtins.cpp`:
```cpp
env->define("time", Value::makeNativeFunction([](const std::vector<Value>& args, const SourceLocation& loc) -> Value {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return Value(static_cast<double>(ms) / 1000.0);
}));
```

### Adding a standard module (e.g. `os` or `json`)
In `src/runtime/builtins.cpp` inside `registerStandardLibraries(Environment* env)`:
```cpp
auto osDict = std::make_shared<DictValue>();

osDict->set("get_env", Value::makeNativeFunction([](const std::vector<Value>& args, const SourceLocation& loc) -> Value {
    if (args.empty() || !args[0].isString()) {
        throw RuntimeError("os.get_env expects a string variable name", loc);
    }
    const char* val = std::getenv(args[0].asString().c_str());
    return val ? Value(std::string(val)) : Value::nil();
}));

osDict->set("platform", Value(
#if defined(_WIN32)
    "windows"
#elif defined(__APPLE__)
    "macos"
#else
    "linux"
#endif
));

env->defineModule("os", Value(osDict));
```

---

## 4. Writing & Running Tests

Nova includes automated test suites located in `tests/`:

- `tests/test_smoke.cpp`: Basic compiler sanity tests.
- `tests/test_lexer.cpp`: Tokenizer validation for keywords, numbers, strings, operators.
- `tests/test_parser.cpp`: AST construction, expression precedence, and syntax rules.
- `tests/test_interpreter.cpp`: Execution tests for OOP, loops, standard modules, closures, and exceptions.

### Running tests locally:
```bash
# Build tests
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure
```

### Adding a new test case:
Add test assertions using the assertion macros in `tests/test_interpreter.cpp`:
```cpp
TEST_CASE("Repeat loop execution") {
    Interpreter interp;
    Value result = interp.runCode("let count = 0\n repeat 5 times count = count + 1 end\n return count");
    ASSERT_EQ(result.asNumber(), 5);
}
```

---

## 5. Version Upgrade & Release Checklist

When upgrading Nova to a new version (e.g., from `0.2.2` to `0.3.0`):

### 1. Update Version Strings across the codebase
1. **`CMakeLists.txt`**:
   ```cmake
   project(nova VERSION 0.3.0 ...)
   ```
2. **`resource.rc`**:
   ```rc
   FILEVERSION     0,3,0,0
   PRODUCTVERSION  0,3,0,0
   VALUE "FileVersion", "0.3.0.0"
   VALUE "ProductVersion", "0.3.0.0"
   ```
3. **`installer/resource.rc`**: Update version blocks to `0,3,0,0`.
4. **`installer/src/main.cpp`**: Update `displayVersion` to `"0.3.0"`.
5. **`src/main.cpp`**: Update CLI `--version` output banner to `0.3.0`.
6. **`CHANGELOG.md`**: Move `[Unreleased]` items under `## [0.3.0] - YYYY-MM-DD`.
7. **`README.md`**: Update version references and badge numbers.

### 2. Verify and Run Test Suite
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### 3. Commit and Create Git Tag
```bash
git add .
git commit -m "chore: release version v0.3.0"
git tag -a v0.3.0 -m "NOVA Release v0.3.0"
git push origin main --tags
```

### 4. Automated GitHub Release
When a `v*` tag is pushed, the `.github/workflows/release.yml` GitHub Action triggers automatically:
- Builds binaries on Windows, Linux, and macOS.
- Generates `nova-windows-x64.zip`, `nova-linux-x64.tar.gz`, `nova-macos-arm64.tar.gz`.
- Bundles the standalone GUI installer `nova-setup.exe`.
- Publishes the GitHub Release with downloadable assets automatically.

---

## 6. Future Development Roadmap

### Planned Enhancements
1. **Bytecode Virtual Machine**:
   - Transition from AST Tree-walk interpreter to high-performance stack-based Bytecode VM (`nova_vm`).
2. **Standard Library Expansion**:
   - `file`: `file.read_text()`, `file.write_text()`, `file.lines()`.
   - `json`: `json.parse()`, `json.stringify()`.
   - `net` / `http`: HTTP requests (`http.get()`, `http.post()`).
3. **Developer Tooling**:
   - Visual Studio Code syntax highlighter (`.vsix` extension).
   - Language Server Protocol (`nova-lsp`) for autocompletion and linting.
   - Interactive REPL enhancements (syntax colorization, multi-line editing, history).
