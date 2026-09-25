#pragma once

#include "source/source_location.hpp"

#include <optional>
#include <string>

namespace nova {

enum class TokenType {
    // Literals
    Number,
    String,
    Identifier,

    // Keywords
    Let,
    If,
    Elif,
    Else,
    End,
    While,
    For,
    In,
    Fn,
    Return,
    And,
    Or,
    Not,
    True,
    False,
    Null,
    Try,
    Catch,
    Is,
    Import,
    As,
    Class,
    Extends,
    Self,
    Static,
    Type,
    Global,
    New,
    Super,
    Break,
    Continue,
    Enum,
    FString,

    // Operators and punctuation
    Plus,
    Minus,
    Star,
    StarStar,
    Slash,
    Percent,
    Equal,
    EqualEqual,
    BangEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Dot,
    DotDot,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Colon,
    Comma,

    Newline,
    Eof,
};

struct Token {
    TokenType type = TokenType::Eof;
    std::string lexeme;
    SourceSpan span{};

    bool is_keyword() const;
    bool is_literal() const;
};

const char* token_type_name(TokenType type);
std::ostream& operator<<(std::ostream& os, TokenType type);

}  // namespace nova
