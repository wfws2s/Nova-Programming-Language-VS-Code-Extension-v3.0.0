# Lexer — Milestone 2

Tokenizes NOVA source into a stream of tokens with source locations.

## Lexer rules

| Feature | Current behavior |
|---------|------------------|
| Comments | `#` to end of line |
| Strings | Double quotes (`"..."`) with `\n`, `\t`, `\r`, `\\`, `\"` escapes |
| Numbers | Integer (`42`) and decimal (`3.14`, `0.5`) |
| Newlines | Emitted as `Newline` tokens (statement separation for parser) |
| Identifiers | `[A-Za-z_][A-Za-z0-9_]*` |

## Public API

```cpp
#include "lexer/lexer.hpp"

nova::Lexer lexer(source);
auto tokens = lexer.tokenize();
if (lexer.had_error()) {
    // lexer.error().format()
}
```

## Files

- `token.hpp` / `token.cpp` — token types and metadata
- `lexer.hpp` / `lexer.cpp` — scanner
- `lexer_error.hpp` / `lexer_error.cpp` — controlled lexical errors
