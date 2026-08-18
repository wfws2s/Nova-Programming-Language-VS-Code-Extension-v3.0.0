#pragma once

#include "source/source_location.hpp"

#include <string>

namespace nova {

struct LexerError {
    std::string message;
    SourceLocation location;

    std::string format() const;
};

}  // namespace nova
