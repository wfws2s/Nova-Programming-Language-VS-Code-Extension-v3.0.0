# NOVA — Language Overview (v0.2.2)

> **Status:** Active language specification for NOVA v0.2.2.

## Philosophy

> Easy to write. Flexible to use. Safe to run. Predictable to understand.

NOVA favors explicit data conversion, readable control flow, robust object-oriented structure, and controlled diagnostics. A program must fail with an informative language error, never a native crash.

## Execution Model

```text
Source Code (.nova)
    ↓
Lexer → Tokens → Parser → AST → Tree-Walk Interpreter → Runtime
```

The current runtime is an efficient C++20 tree-walk interpreter with extensible modules and lexical environments.

## Core Language Decisions

| Topic | Decision |
|-------|----------|
| Variables | `name = value` creates or reassigns a mutable variable. |
| Constants | `let name = value` creates an immutable binding in the current block. |
| Global Variables | `global name` binds assignments in functions/methods to global scope. |
| Classes & OOP | `class Name ... end` with constructors `fn init(self, ...)`, instance methods, static methods, and inheritance (`extends`). |
| Supercalls | `super.init(...)` and `super.method(...)` dispatch to parent class hierarchy with current `self`. |
| Modules & Imports | `import "path" as alias` loads `.nova` files via direct relative/absolute paths; `import module as alias` loads standard modules or local files. |
| Type Inspection | `type(x)` or `type x` returns the canonical type string (`"Number"`, `"String"`, `"Boolean"`, `"List"`, `"Dictionary"`, `"Function"`, `"Class"`, `"Instance"`, `"Null"`). |
| Standard Library | Built-in modules include `random`, `math`, `string`, `list`, and `dict`. |
| Block Terminations | All blocks (`class`, `fn`, `if`, `while`, `for`, `try`) terminate with `end`. |
| Reserved Words | Documented keywords cannot be identifiers. Built-in functions (`print`, `input`, `len`, etc.) are normal environment bindings that can be shadowed. |
| Type Conversion | Arithmetic and `+` never coerce values implicitly. |
| Exponentiation | `**` is right-associative: `2 ** 3 ** 2` is `512`. |
| Dictionary Keys | `String` only, for predictable lookup and serialization. |
| Empty Collections | `[]` and `{}` are truthy. Only `false` and `null` are falsey. |
| Indexing & Slicing | Zero-based indexing with negative index support; `list[start..end]` slicing. |
| Function Returns | A function with no executed `return` yields `null`. |

---

## Reserved Keywords

The following keywords cannot be used as variable or function names:
`let`, `if`, `elif`, `else`, `end`, `while`, `for`, `in`, `fn`, `return`, `and`, `or`, `not`, `true`, `false`, `null`, `try`, `catch`, `is`, `import`, `as`, `class`, `extends`, `self`, `static`, `type`, `global`, `new`, `super`.

Using any reserved keyword as a variable or binding name reports `ReservedNameError`.

---

## I/O and Standard Library

- `print(...)`: Prints zero or more arguments separated by spaces.
- `input(prompt)`: Reads standard input and returns a `String`.
- `read(prompt)`: Reads standard input and parses literals recursively (`null`, bool, number, list, dict), falling back to `String`.
- `type(x)` / `typeof(x)`: Returns the canonical type name as a `String`.
- `number(x)`, `string(x)`, `list(x)`: Explicit type converters.
- `split(text, delim)`: Splits string by delimiter into a `List`.
- `append(list, val)` and `pop(list)`: List manipulation built-ins.
- `keys(dict)` and `values(dict)`: Dictionary inspection built-ins.
- Standard modules via `import`:
  - `random`: `random()`, `randint()`, `choice()`, `shuffle()`, `uniform()`, `range()`
  - `math`: `sqrt`, `pow`, `floor`, `ceil`, `round`, `abs`, `sin`, `cos`, `tan`, `min`, `max`, `pi`, `e`
  - `string`: `to_upper`, `to_lower`, `trim`, `starts_with`, `ends_with`, `contains`, `join`
  - `list`: `contains`, `index_of`, `reverse`
  - `dict`: `has_key`
