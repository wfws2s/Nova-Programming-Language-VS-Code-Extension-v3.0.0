# NOVA — Error Diagnostics Specification

> **Status:** Active diagnostics policy. Categories labelled **Full release** are approved additions.

## 1. Safety Guarantee

Invalid NOVA code must produce a controlled diagnostic, never a segmentation fault, access violation, unhandled C++ exception, or silent process termination.

## 2. Error Categories

| Category | Trigger | Example |
|----------|---------|---------|
| `LexerError` | Invalid character or unterminated string | `Unterminated string literal` |
| `SyntaxError` | Invalid structure or missing token | `Expected 'end' after if statement` |
| `ReservedNameError` | **Full release:** keyword used as a binding name | `cannot use reserved word 'if' as a variable name` |
| `ConstantError` | Reassigning a `let` binding | `cannot reassign constant 'age'` |
| `NameError` | Unknown binding | `Undefined variable 'x'` |
| `TypeError` | Invalid type for an operator or API | `Cannot add Number and String` |
| `IndexError` | Invalid List/String index or empty `pop` | `List index out of bounds: 5` |
| `KeyError` | Missing Dictionary key | `Key not found in dictionary: "country"` |
| `ArgumentError` | Invalid function arity | `input() takes exactly 1 argument (0 given)` |
| `RuntimeError` | Division by zero, execution guard, or invalid operation | `Division by zero` |

## 3. Diagnostic Format

```text
ConstantError: cannot reassign constant 'age'
  --> script.nova:2:1
   |
 2 | age = 16
   | ^~~~~~~~

Hint: 'age' was declared as a constant with 'let age = ...'.
```

Every diagnostic contains its category, message, filename, line, column, source excerpt, and an actionable hint when available.

## 4. Stack Traces — Full release

An uncaught runtime error includes a stack trace after the primary diagnostic. Frames are ordered from the failing location outward:

```text
Stack trace:
  at divide (math.nova:4:12)
  called from calculate (main.nova:12:5)
  called from <main> (main.nova:18:1)
```

Native implementation frames are never shown. A caught error exposes this same structured data through `error["stack"]`.
