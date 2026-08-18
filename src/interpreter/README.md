# Interpreter

The tree-walk interpreter is implemented. It evaluates AST nodes with lexical scope, closures, functions, control flow, lists, dictionaries, and built-ins.

Runtime failures are surfaced as NOVA diagnostics rather than native exceptions. Full-release features documented in `docs/` require corresponding interpreter support and regression tests before they are available to programs.
