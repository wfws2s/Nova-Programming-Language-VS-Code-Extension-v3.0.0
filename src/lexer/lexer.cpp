#include "lexer/lexer.hpp"

#include <cctype>
#include <utility>

namespace nova {

namespace {

bool is_identifier_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool is_identifier_part(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

}  // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!had_error()) {
        Token token = scan_token();
        tokens.push_back(token);
        if (token.type == TokenType::Eof) {
            break;
        }
    }

    return tokens;
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

char Lexer::peek() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

char Lexer::advance() {
    if (is_at_end()) {
        return '\0';
    }

    char c = source_[current_++];
    ++location_.offset;

    if (c == '\n') {
        ++location_.line;
        location_.column = 1;
    } else {
        ++location_.column;
    }

    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
            continue;
        }
        break;
    }
}

void Lexer::skip_comment() {
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
}

Token Lexer::make_token(TokenType type, SourceLocation start, std::string lexeme) {
    return Token{type, std::move(lexeme), SourceSpan{start, location_}};
}

Token Lexer::scan_token() {
    skip_whitespace();

    SourceLocation start = location_;

    if (is_at_end()) {
        return make_token(TokenType::Eof, start, "");
    }

    char c = advance();

    if (c == '\n') {
        return make_token(TokenType::Newline, start, "\n");
    }

    if (is_identifier_start(c)) {
        return scan_identifier(start);
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number(start);
    }

    switch (c) {
        case '#':
            skip_comment();
            return scan_token();
        case '"':
            return scan_string(start);
        case '(':
            return make_token(TokenType::LeftParen, start, "(");
        case ')':
            return make_token(TokenType::RightParen, start, ")");
        case '[':
            return make_token(TokenType::LeftBracket, start, "[");
        case ']':
            return make_token(TokenType::RightBracket, start, "]");
        case '{':
            return make_token(TokenType::LeftBrace, start, "{");
        case '}':
            return make_token(TokenType::RightBrace, start, "}");
        case ':':
            return make_token(TokenType::Colon, start, ":");
        case '.':
            if (match('.')) {
                return make_token(TokenType::DotDot, start, "..");
            }
            return make_token(TokenType::Dot, start, ".");
        case ',':
            return make_token(TokenType::Comma, start, ",");
        case '+':
            return make_token(TokenType::Plus, start, "+");
        case '-':
            return make_token(TokenType::Minus, start, "-");
        case '*':
            if (match('*')) {
                return make_token(TokenType::StarStar, start, "**");
            }
            return make_token(TokenType::Star, start, "*");
        case '/':
            return make_token(TokenType::Slash, start, "/");
        case '%':
            return make_token(TokenType::Percent, start, "%");
        case '=':
            if (match('=')) {
                return make_token(TokenType::EqualEqual, start, "==");
            }
            return make_token(TokenType::Equal, start, "=");
        case '!':
            if (match('=')) {
                return make_token(TokenType::BangEqual, start, "!=");
            }
            report_error("Unexpected character '!' (expected '!=')");
            return make_token(TokenType::Eof, start, "");
        case '>':
            if (match('=')) {
                return make_token(TokenType::GreaterEqual, start, ">=");
            }
            return make_token(TokenType::Greater, start, ">");
        case '<':
            if (match('=')) {
                return make_token(TokenType::LessEqual, start, "<=");
            }
            return make_token(TokenType::Less, start, "<");
        default:
            report_error(std::string("Unexpected character '") + c + "'");
            return make_token(TokenType::Eof, start, "");
    }
}

Token Lexer::scan_number(SourceLocation start) {
    --current_;
    --location_.offset;
    if (location_.column > 1) {
        --location_.column;
    }

    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        advance();  // consume '.'
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    std::string lexeme = source_.substr(start.offset, location_.offset - start.offset);
    return make_token(TokenType::Number, start, std::move(lexeme));
}

Token Lexer::scan_string(SourceLocation start) {
    std::string value;

    while (!is_at_end() && peek() != '"') {
        char c = advance();
        if (c == '\n') {
            report_error("Unterminated string literal");
            return make_token(TokenType::Eof, start, "");
        }
        if (c == '\\') {
            if (is_at_end()) {
                report_error("Unterminated string literal");
                return make_token(TokenType::Eof, start, "");
            }
            char escaped = advance();
            switch (escaped) {
                case 'n':
                    value.push_back('\n');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case '\\':
                    value.push_back('\\');
                    break;
                case '"':
                    value.push_back('"');
                    break;
                default:
                    report_error(std::string("Invalid escape sequence '\\") + escaped + "'");
                    return make_token(TokenType::Eof, start, "");
            }
            continue;
        }
        value.push_back(c);
    }

    if (is_at_end()) {
        report_error("Unterminated string literal");
        return make_token(TokenType::Eof, start, "");
    }

    advance();  // closing quote
    return make_token(TokenType::String, start, std::move(value));
}

Token Lexer::scan_identifier(SourceLocation start) {
    --current_;
    --location_.offset;
    if (location_.column > 1) {
        --location_.column;
    }

    while (is_identifier_part(peek())) {
        advance();
    }

    std::string text = source_.substr(start.offset, location_.offset - start.offset);
    if (auto keyword = lookup_keyword(text)) {
        return make_token(*keyword, start, text);
    }
    return make_token(TokenType::Identifier, start, text);
}

std::optional<TokenType> Lexer::lookup_keyword(const std::string& text) const {
    if (text == "let") return TokenType::Let;
    if (text == "if") return TokenType::If;
    if (text == "elif") return TokenType::Elif;
    if (text == "else") return TokenType::Else;
    if (text == "end") return TokenType::End;
    if (text == "while") return TokenType::While;
    if (text == "for") return TokenType::For;
    if (text == "in") return TokenType::In;
    if (text == "fn") return TokenType::Fn;
    if (text == "return") return TokenType::Return;
    if (text == "and") return TokenType::And;
    if (text == "or") return TokenType::Or;
    if (text == "not") return TokenType::Not;
    if (text == "true") return TokenType::True;
    if (text == "false") return TokenType::False;
    if (text == "null") return TokenType::Null;
    if (text == "try") return TokenType::Try;
    if (text == "catch") return TokenType::Catch;
    if (text == "is") return TokenType::Is;
    if (text == "import") return TokenType::Import;
    if (text == "as") return TokenType::As;
    if (text == "class") return TokenType::Class;
    if (text == "extends") return TokenType::Extends;
    if (text == "self") return TokenType::Self;
    if (text == "static") return TokenType::Static;
    if (text == "type") return TokenType::Type;
    if (text == "global") return TokenType::Global;
    if (text == "new") return TokenType::New;
    if (text == "super") return TokenType::Super;
    return std::nullopt;
}

void Lexer::report_error(std::string message) {
    if (!error_.has_value()) {
        error_ = LexerError{std::move(message), location_};
    }
}

}  // namespace nova
