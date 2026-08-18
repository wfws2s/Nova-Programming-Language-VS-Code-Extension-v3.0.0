# Parser

The parser is implemented as a Pratt parser that produces NOVA AST nodes. It handles statements, functions, control flow, calls, list/dictionary literals, indexing, assignment, ranges, and right-associative exponentiation.

It reports controlled `SyntaxError` diagnostics with source spans. Planned Full-release syntax is documented in `docs/SYNTAX.md`; it must not be treated as implemented until parser support and tests are added.
