#pragma once

#include "lexer/token.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace nova::test {

inline int failures = 0;

inline void expect_true(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

inline void expect_eq(const char* actual, const char* expected, const char* message) {
    if (std::string_view(actual) != std::string_view(expected)) {
        std::cerr << "FAIL: " << message << " (expected \"" << expected << "\", got \"" << actual
                  << "\")\n";
        ++failures;
    }
}

inline void expect_eq(std::string_view actual, std::string_view expected, const char* message) {
    if (actual != expected) {
        std::cerr << "FAIL: " << message << " (expected \"" << expected << "\", got \"" << actual
                  << "\")\n";
        ++failures;
    }
}

template <typename T, typename U>
inline void expect_eq(const T& actual, const U& expected, const char* message) {
    if (actual != expected) {
        std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
        ++failures;
    }
}

inline void expect_token_type(const Token& token, TokenType expected, const char* message) {
    if (token.type != expected) {
        std::cerr << "FAIL: " << message << " (expected " << token_type_name(expected) << ", got "
                  << token_type_name(token.type) << ")\n";
        ++failures;
    }
}

inline void expect_token_lexeme(const Token& token, const std::string& expected,
                                const char* message) {
    if (token.lexeme != expected) {
        std::cerr << "FAIL: " << message << " (expected lexeme \"" << expected << "\", got \""
                  << token.lexeme << "\")\n";
        ++failures;
    }
}

inline void expect_token_span(const Token& token, int start_line, int start_column, int end_line,
                              int end_column, const char* message) {
    if (token.span.start.line != start_line || token.span.start.column != start_column ||
        token.span.end.line != end_line || token.span.end.column != end_column) {
        std::cerr << "FAIL: " << message << " (expected span " << start_line << ":" << start_column
                  << " -> " << end_line << ":" << end_column << ", got " << token.span.start.line
                  << ":" << token.span.start.column << " -> " << token.span.end.line << ":"
                  << token.span.end.column << ")\n";
        ++failures;
    }
}

inline void expect_token_count(const std::vector<Token>& tokens, std::size_t expected,
                               const char* message) {
    if (tokens.size() != expected) {
        std::cerr << "FAIL: " << message << " (expected " << expected << " tokens, got "
                  << tokens.size() << ")\n";
        ++failures;
    }
}

inline int finish(const char* suite_name) {
    if (failures == 0) {
        std::cout << suite_name << ": all tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << suite_name << ": " << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}

}  // namespace nova::test
