# NOVA — Types Specification (v0.2.2)

> **Status:** Active type rules for NOVA v0.2.2.

---

## 1. Core Types

| Type | Description | Example |
|------|-------------|---------|
| `Number` | IEEE-754 64-bit floating-point value | `42`, `3.14`, `-10` |
| `String` | UTF-8 text | `"Hello"`, `"Value: {x}"` |
| `Boolean` | Logical value | `true`, `false` |
| `Null` | Absence of a value | `null` |
| `List` | Ordered, mutable collection | `[10, 20, 30]` |
| `Dictionary` | Mutable String-keyed mapping | `{"name": "Nova", "ver": 2}` |
| `Function` | User-defined or built-in callable | `fn(x) return x end` |
| `Class` | Class blueprint definition | `class Person ... end` |
| `Instance` | Object instance of a class | `Person("Alice", 30)` |

`type(value)`, `type value`, and `typeof(value)` return exactly one of these names as a `String`.

---

## 2. Type Checking and Conversions

NOVA enforces explicit data conversions. Operators do not implicitly coerce types:

```nova
10 + "20"       # Raises TypeError
"10" + 20       # Raises TypeError
```

Use explicit built-in conversion functions:

```nova
number("15")    # 15.0
string(15)      # "15"
list("abc")     # ["a", "b", "c"]
list({"a": 1})  # ["a"]
```

---

## 3. Truthiness

Only `false` and `null` are falsey. Every other value is truthy, including `0`, `""`, `[]`, and `{}`.

---

## 4. Built-in Function Reference

| Function | Return Type | Description |
|----------|-------------|-------------|
| `print(...)` | `Null` | Prints all arguments separated by spaces with trailing newline. |
| `input(prompt)` | `String` | Reads a line of input from standard input as a String. |
| `read(prompt)` | Any | Reads standard input and parses literals recursively (`null`, bool, number, list, dict), falling back to String. |
| `type(val)` | `String` | Returns canonical type name of value. |
| `typeof(val)` | `String` | Alias to `type(val)`. |
| `number(val)` | `Number` | Parses String or converts Boolean to Number. |
| `string(val)` | `String` | Formats any value to a String. |
| `list(val)` | `List` | Converts strings, dictionaries, or lists to a List. |
| `len(val)` | `Number` | Length of String, List, or Dictionary. |
| `split(str, delim)` | `List` | Splits string by delimiter. |
| `append(list, val)` | `Null` | Appends item to list in place. |
| `pop(list)` | Value | Removes and returns the last element of list. |
| `keys(dict)` | `List` | Returns list of dictionary keys. |
| `values(dict)` | `List` | Returns list of dictionary values. |

---

## 5. Standard Module Reference

### `random`
- `random()`: Float in `[0.0, 1.0)`.
- `randint(a, b)`: Integer in `[a, b]` inclusive.
- `choice(list)`: Uniformly picks an element from `list`.
- `shuffle(list)`: Shuffles `list` in place.
- `uniform(a, b)`: Float in `[a, b]`.
- `range(a, b)`: Integer in `[a, b)`.

### `math`
- `sqrt(x)`, `pow(x, y)`, `floor(x)`, `ceil(x)`, `round(x)`, `abs(x)`
- `sin(x)`, `cos(x)`, `tan(x)`
- `min(...)`, `max(...)`
- `pi`, `e`

### `string`
- `to_upper(s)`, `to_lower(s)`, `trim(s)`
- `starts_with(s, prefix)`, `ends_with(s, suffix)`, `contains(s, sub)`
- `join(list, delim)`

### `list`
- `contains(list, item)`, `index_of(list, item)`, `reverse(list)`

### `dict`
- `has_key(dict, key)`
