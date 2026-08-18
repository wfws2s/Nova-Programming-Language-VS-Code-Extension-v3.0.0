#include "../src/lexer/lexer.hpp"

#include "test_helpers.hpp"

using namespace nova;
using namespace nova::test;

void test_empty_source() {
    Lexer lexer("");
    auto tokens = lexer.tokenize();
    expect_token_count(tokens, 1, "empty source token count");
    expect_token_type(tokens[0], TokenType::Eof, "empty source eof");
    expect_true(!lexer.had_error(), "empty source no error");
}

void test_keywords() {
    Lexer lexer("let if elif else end while for in fn return and or not true false null try catch is import as class extends self static type global new super");
    auto tokens = lexer.tokenize();

    const TokenType expected[] = {
        TokenType::Let,    TokenType::If,      TokenType::Elif,   TokenType::Else,   TokenType::End,
        TokenType::While,  TokenType::For,     TokenType::In,     TokenType::Fn,
        TokenType::Return, TokenType::And,     TokenType::Or,     TokenType::Not,
        TokenType::True,   TokenType::False,   TokenType::Null,   TokenType::Try,
        TokenType::Catch,  TokenType::Is,      TokenType::Import, TokenType::As,
        TokenType::Class,  TokenType::Extends, TokenType::Self,   TokenType::Static,
        TokenType::Type,   TokenType::Global,  TokenType::New,    TokenType::Super,
        TokenType::Eof,
    };

    expect_token_count(tokens, 30, "keyword count");
    for (std::size_t i = 0; i < 29; ++i) {
        expect_token_type(tokens[i], expected[i], "keyword type");
    }
    expect_true(!lexer.had_error(), "keywords no error");
}

void test_identifiers() {
    Lexer lexer("foo _bar baz123");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 4, "identifier count");
    expect_token_type(tokens[0], TokenType::Identifier, "identifier type");
    expect_token_lexeme(tokens[0], "foo", "identifier lexeme");
    expect_token_lexeme(tokens[1], "_bar", "identifier lexeme underscore");
    expect_token_lexeme(tokens[2], "baz123", "identifier lexeme digits");
}

void test_numbers() {
    Lexer lexer("0 42 3.14 0.5");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 5, "number count");
    expect_token_type(tokens[0], TokenType::Number, "number type");
    expect_token_lexeme(tokens[0], "0", "zero");
    expect_token_lexeme(tokens[1], "42", "integer");
    expect_token_lexeme(tokens[2], "3.14", "float");
    expect_token_lexeme(tokens[3], "0.5", "leading zero decimal");
}

void test_strings() {
    Lexer lexer(R"("hello" "line\nbreak" "quote\"mark")");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 4, "string count");
    expect_token_type(tokens[0], TokenType::String, "string type");
    expect_token_lexeme(tokens[0], "hello", "plain string");
    expect_token_lexeme(tokens[1], "line\nbreak", "escaped newline");
    expect_token_lexeme(tokens[2], "quote\"mark", "escaped quote");
}

void test_operators() {
    Lexer lexer("+ - * / % == != > < >= <= =");
    auto tokens = lexer.tokenize();

    const TokenType expected[] = {
        TokenType::Plus,         TokenType::Minus,        TokenType::Star,
        TokenType::Slash,        TokenType::Percent,      TokenType::EqualEqual,
        TokenType::BangEqual,    TokenType::Greater,      TokenType::Less,
        TokenType::GreaterEqual, TokenType::LessEqual,    TokenType::Equal,
        TokenType::Eof,
    };

    expect_token_count(tokens, 13, "operator count");
    for (std::size_t i = 0; i < 12; ++i) {
        expect_token_type(tokens[i], expected[i], "operator type");
    }
}

void test_punctuation() {
    Lexer lexer("(a, b)");
    auto tokens = lexer.tokenize();

    expect_token_type(tokens[0], TokenType::LeftParen, "left paren");
    expect_token_type(tokens[1], TokenType::Identifier, "identifier in parens");
    expect_token_type(tokens[2], TokenType::Comma, "comma");
    expect_token_type(tokens[3], TokenType::Identifier, "second identifier");
    expect_token_type(tokens[4], TokenType::RightParen, "right paren");
}

void test_comments_and_newlines() {
    Lexer lexer("let x = 10 # assignment\nlet y = 20");
    auto tokens = lexer.tokenize();

    expect_token_type(tokens[0], TokenType::Let, "comment line let");
    expect_token_type(tokens[1], TokenType::Identifier, "comment line identifier");
    expect_token_type(tokens[2], TokenType::Equal, "comment line equal");
    expect_token_type(tokens[3], TokenType::Number, "comment line number");
    expect_token_type(tokens[4], TokenType::Newline, "comment line newline");
    expect_token_type(tokens[5], TokenType::Let, "second line let");
    expect_true(!lexer.had_error(), "comments no error");
}

void test_source_locations() {
    Lexer lexer("let\nx");
    auto tokens = lexer.tokenize();

    expect_token_span(tokens[0], 1, 1, 1, 4, "let span");
    expect_token_span(tokens[1], 1, 4, 2, 1, "newline span");
    expect_token_span(tokens[2], 2, 1, 2, 2, "x span");
}

void test_sample_program() {
    Lexer lexer(R"(let x = 10

if x > 5
    print("Hello")
end
)");
    auto tokens = lexer.tokenize();
    expect_true(tokens.size() > 10, "sample program produces tokens");
    expect_token_type(tokens[0], TokenType::Let, "sample let");
    expect_true(!lexer.had_error(), "sample program no error");
}

void test_unterminated_string_error() {
    Lexer lexer("\"hello");
    auto tokens = lexer.tokenize();
    expect_true(lexer.had_error(), "unterminated string reports error");
    expect_eq(lexer.error().message, "Unterminated string literal", "unterminated string message");
    expect_true(tokens.back().type == TokenType::Eof || tokens.size() >= 1,
                "unterminated string partial tokens");
}

void test_invalid_escape_error() {
    Lexer lexer(R"("\q")");
    lexer.tokenize();
    expect_true(lexer.had_error(), "invalid escape reports error");
    expect_eq(lexer.error().message, "Invalid escape sequence '\\q'", "invalid escape message");
}

void test_unexpected_character_error() {
    Lexer lexer("@");
    lexer.tokenize();
    expect_true(lexer.had_error(), "unexpected char reports error");
    expect_eq(lexer.error().message, "Unexpected character '@'", "unexpected char message");
}

void test_brackets_and_ranges() {
    Lexer lexer("[1, 2] 0..10");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 9, "brackets and ranges count");
    expect_token_type(tokens[0], TokenType::LeftBracket, "left bracket");
    expect_token_type(tokens[1], TokenType::Number, "first element");
    expect_token_type(tokens[2], TokenType::Comma, "comma");
    expect_token_type(tokens[3], TokenType::Number, "second element");
    expect_token_type(tokens[4], TokenType::RightBracket, "right bracket");
    expect_token_type(tokens[5], TokenType::Number, "range start");
    expect_token_lexeme(tokens[5], "0", "range start 0");
    expect_token_type(tokens[6], TokenType::DotDot, "dot dot");
    expect_token_type(tokens[7], TokenType::Number, "range end");
    expect_token_lexeme(tokens[7], "10", "range end 10");
    expect_token_type(tokens[8], TokenType::Eof, "eof");
    expect_true(!lexer.had_error(), "brackets and ranges no error");
}

void test_bang_without_equal_error() {
    Lexer lexer("!");
    lexer.tokenize();
    expect_true(lexer.had_error(), "bare bang reports error");
    expect_eq(lexer.error().message, "Unexpected character '!' (expected '!=')",
              "bare bang message");
}

void test_dot_token() {
    Lexer lexer("a.b");
    auto tokens = lexer.tokenize();
    expect_token_count(tokens, 4, "dot token count");
    expect_token_type(tokens[0], TokenType::Identifier, "a");
    expect_token_type(tokens[1], TokenType::Dot, ".");
    expect_token_type(tokens[2], TokenType::Identifier, "b");
    expect_token_type(tokens[3], TokenType::Eof, "eof");
    expect_true(!lexer.had_error(), "dot token no error");
}

void test_power_operator() {
    Lexer lexer("2 ** 10 * 3");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 6, "power operator count");
    expect_token_type(tokens[0], TokenType::Number, "2");
    expect_token_type(tokens[1], TokenType::StarStar, "**");
    expect_token_lexeme(tokens[1], "**", "lexeme **");
    expect_token_type(tokens[2], TokenType::Number, "10");
    expect_token_type(tokens[3], TokenType::Star, "*");
    expect_token_lexeme(tokens[3], "*", "lexeme *");
    expect_token_type(tokens[4], TokenType::Number, "3");
    expect_token_type(tokens[5], TokenType::Eof, "eof");
    expect_true(!lexer.had_error(), "power operator no error");
}

void test_dictionary_tokens() {
    Lexer lexer(R"({"name": "JIMY", "age": 15})");
    auto tokens = lexer.tokenize();

    expect_token_count(tokens, 10, "dict tokens count");
    expect_token_type(tokens[0], TokenType::LeftBrace, "{");
    expect_token_type(tokens[1], TokenType::String, "key name");
    expect_token_type(tokens[2], TokenType::Colon, ":");
    expect_token_type(tokens[3], TokenType::String, "val JIMY");
    expect_token_type(tokens[4], TokenType::Comma, ",");
    expect_token_type(tokens[5], TokenType::String, "key age");
    expect_token_type(tokens[6], TokenType::Colon, ":");
    expect_token_type(tokens[7], TokenType::Number, "val 15");
    expect_token_type(tokens[8], TokenType::RightBrace, "}");
    expect_token_type(tokens[9], TokenType::Eof, "eof");
    expect_true(!lexer.had_error(), "dict tokens no error");
}

int main() {
    test_empty_source();
    test_keywords();
    test_identifiers();
    test_numbers();
    test_strings();
    test_operators();
    test_punctuation();
    test_brackets_and_ranges();
    test_comments_and_newlines();
    test_source_locations();
    test_sample_program();
    test_unterminated_string_error();
    test_invalid_escape_error();
    test_unexpected_character_error();
    test_bang_without_equal_error();
    test_dot_token();
    test_power_operator();
    test_dictionary_tokens();

    return finish("test_lexer");
}
