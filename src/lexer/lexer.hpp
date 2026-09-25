#pragma once

#include "lexer/lexer_error.hpp"
#include "lexer/token.hpp"

#include <optional>
#include <string>
#include <vector>

namespace nova {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();
    bool had_error() const { return error_.has_value(); }
    const LexerError& error() const { return *error_; }

private:
    bool is_at_end() const;
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);

    void skip_whitespace();
    void skip_comment();

    Token make_token(TokenType type, SourceLocation start, std::string lexeme);
    Token scan_token();

    Token scan_number(SourceLocation start);
    Token scan_string(SourceLocation start);
    Token scan_fstring(SourceLocation start);
    Token scan_triple_string(SourceLocation start);
    Token scan_identifier(SourceLocation start);

    std::optional<TokenType> lookup_keyword(const std::string& text) const;
    void report_error(std::string message);

    std::string source_;
    std::size_t current_ = 0;
    SourceLocation location_{1, 1, 0};
    std::optional<LexerError> error_;
};

}  // namespace nova
