#include "lexer/token.hpp"

#include <ostream>

namespace nova {

bool Token::is_keyword() const {
    switch (type) {
        case TokenType::Let:
        case TokenType::If:
        case TokenType::Elif:
        case TokenType::Else:
        case TokenType::End:
        case TokenType::While:
        case TokenType::For:
        case TokenType::In:
        case TokenType::Fn:
        case TokenType::Return:
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Not:
        case TokenType::True:
        case TokenType::False:
        case TokenType::Null:
        case TokenType::Try:
        case TokenType::Catch:
        case TokenType::Is:
        case TokenType::Import:
        case TokenType::As:
        case TokenType::Class:
        case TokenType::Extends:
        case TokenType::Self:
        case TokenType::Static:
        case TokenType::Type:
        case TokenType::Global:
        case TokenType::New:
        case TokenType::Super:
        case TokenType::Break:
        case TokenType::Continue:
        case TokenType::Enum:
            return true;
        default:
            return false;
    }
}

bool Token::is_literal() const {
    return type == TokenType::Number || type == TokenType::String || type == TokenType::True ||
           type == TokenType::False || type == TokenType::Null;
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Number:
            return "Number";
        case TokenType::String:
            return "String";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::Let:
            return "Let";
        case TokenType::If:
            return "If";
        case TokenType::Elif:
            return "Elif";
        case TokenType::Else:
            return "Else";
        case TokenType::End:
            return "End";
        case TokenType::While:
            return "While";
        case TokenType::For:
            return "For";
        case TokenType::In:
            return "In";
        case TokenType::Fn:
            return "Fn";
        case TokenType::Return:
            return "Return";
        case TokenType::And:
            return "And";
        case TokenType::Or:
            return "Or";
        case TokenType::Not:
            return "Not";
        case TokenType::True:
            return "True";
        case TokenType::False:
            return "False";
        case TokenType::Null:
            return "Null";
        case TokenType::Try:
            return "Try";
        case TokenType::Catch:
            return "Catch";
        case TokenType::Is:
            return "Is";
        case TokenType::Import:
            return "Import";
        case TokenType::As:
            return "As";
        case TokenType::Class:
            return "Class";
        case TokenType::Extends:
            return "Extends";
        case TokenType::Self:
            return "Self";
        case TokenType::Static:
            return "Static";
        case TokenType::Type:
            return "Type";
        case TokenType::Global:
            return "Global";
        case TokenType::New:
            return "New";
        case TokenType::Super:
            return "Super";
        case TokenType::Break:
            return "Break";
        case TokenType::Continue:
            return "Continue";
        case TokenType::Enum:
            return "Enum";
        case TokenType::FString:
            return "FString";
        case TokenType::Plus:
            return "Plus";
        case TokenType::Minus:
            return "Minus";
        case TokenType::Star:
            return "Star";
        case TokenType::StarStar:
            return "StarStar";
        case TokenType::Slash:
            return "Slash";
        case TokenType::Percent:
            return "Percent";
        case TokenType::Equal:
            return "Equal";
        case TokenType::EqualEqual:
            return "EqualEqual";
        case TokenType::BangEqual:
            return "BangEqual";
        case TokenType::Greater:
            return "Greater";
        case TokenType::GreaterEqual:
            return "GreaterEqual";
        case TokenType::Less:
            return "Less";
        case TokenType::LessEqual:
            return "LessEqual";
        case TokenType::Dot:
            return "Dot";
        case TokenType::DotDot:
            return "DotDot";
        case TokenType::LeftParen:
            return "LeftParen";
        case TokenType::RightParen:
            return "RightParen";
        case TokenType::LeftBracket:
            return "LeftBracket";
        case TokenType::RightBracket:
            return "RightBracket";
        case TokenType::LeftBrace:
            return "LeftBrace";
        case TokenType::RightBrace:
            return "RightBrace";
        case TokenType::Colon:
            return "Colon";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Newline:
            return "Newline";
        case TokenType::Eof:
            return "Eof";
    }
    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, TokenType type) {
    return os << token_type_name(type);
}

}  // namespace nova
