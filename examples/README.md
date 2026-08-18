# NOVA Examples (v0.2.2)

The examples in this directory demonstrate features of the NOVA language:

- **`oop_and_features_demo.nova`**: Complete demonstration of OOP (classes, inheritance with `extends`, `init` constructor, `self`, `super`, `static fn`), external file imports via direct path with `as`, `type` inspection, `random` standard library, and `global` variable scoping.
- **`math_helper.nova`**: External module defining classes and functions to be imported into other files.
- **`full_release_features.nova`**: Demonstrates standard library utilities (`math`, `string`, `list`, `dict`), list slicing, dynamic input parsing (`read`), string interpolation, and exception handling (`try ... catch`).
- **`hello.nova`**: Simple greetings and basic arithmetic.
- **`arrays.nova`**: Working with lists and negative indexing.
- **`fibonacci.nova`**: Recursive functions and benchmarking.
- **`error_demo.nova`**: Rich error diagnostics.

To run an example:

```bash
# On Linux/macOS:
./build/nova examples/oop_and_features_demo.nova

# On Windows:
.\build\nova.exe examples\oop_and_features_demo.nova
```
