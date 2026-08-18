#pragma once

#include <string>

namespace nova {

struct SourceLocation {
    int line = 1;
    int column = 1;
    int offset = 0;

    bool operator==(const SourceLocation& other) const {
        return line == other.line && column == other.column && offset == other.offset;
    }
};

struct SourceSpan {
    SourceLocation start{};
    SourceLocation end{};

    bool operator==(const SourceSpan& other) const {
        return start == other.start && end == other.end;
    }
};

}  // namespace nova
