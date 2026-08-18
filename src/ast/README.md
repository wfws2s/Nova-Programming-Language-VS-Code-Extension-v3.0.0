# AST

The AST is implemented and is the shared representation between the Pratt parser and tree-walk interpreter.

It contains expressions for literals, variables, assignment, calls, lists, dictionaries, ranges, indexing, and operators; and statements for bindings, blocks, control flow, functions, and returns.

The visitor interface is the execution boundary used by `Interpreter`. New language syntax must first be specified in `docs/`, then added to this model and its visitor contract.
