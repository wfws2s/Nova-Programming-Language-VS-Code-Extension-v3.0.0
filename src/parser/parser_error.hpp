#pragma once

#include "source/source_location.hpp"

#include <string>

namespace nova {

struct ParserError {
    std::string message;
    SourceLocation location{};
    SourceSpan span{};
    std::string category = "SyntaxError";
    std::string hint = "";
};

}  // namespace nova
