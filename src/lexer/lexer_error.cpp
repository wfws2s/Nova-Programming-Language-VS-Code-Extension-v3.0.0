#include "lexer/lexer_error.hpp"

#include <sstream>

namespace nova {

std::string LexerError::format() const {
    std::ostringstream out;
    out << "LexerError at " << location.line << ":" << location.column << ": " << message;
    return out.str();
}

}  // namespace nova
