#include "interpreter/diagnostics.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

namespace nova {

namespace {

std::vector<std::string_view> split_lines(std::string_view source) {
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    while (start < source.size()) {
        std::size_t end = source.find('\n', start);
        if (end == std::string_view::npos) {
            lines.push_back(source.substr(start));
            break;
        }
        lines.push_back(source.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

}  // namespace

std::string Diagnostics::format_error(
    std::string_view category,
    std::string_view message,
    SourceSpan span,
    std::string_view source_code,
    std::string_view filename,
    std::string_view hint
) {
    std::ostringstream ss;
    int line_num = span.start.line > 0 ? span.start.line : 1;
    int col_num = span.start.column > 0 ? span.start.column : 1;

    ss << category << ": " << message << "\n";
    ss << "  --> " << filename << ":" << line_num << ":" << col_num << "\n";

    auto lines = split_lines(source_code);
    if (!lines.empty() && static_cast<std::size_t>(line_num - 1) < lines.size()) {
        std::string_view line_content = lines[line_num - 1];
        std::string line_str = std::to_string(line_num);
        std::string padding(line_str.size(), ' ');

        ss << "   " << padding << " |\n";
        ss << "   " << line_str << " | " << line_content << "\n";
        ss << "   " << padding << " | ";

        int col_start = std::max(1, col_num);
        for (int i = 1; i < col_start; ++i) {
            ss << ' ';
        }

        int underline_len = 1;
        if (span.end.line == span.start.line && span.end.column > span.start.column) {
            underline_len = span.end.column - span.start.column;
        }

        ss << '^';
        for (int i = 1; i < underline_len; ++i) {
            ss << '~';
        }
        ss << "\n";
    }

    if (!hint.empty()) {
        ss << "\nHint: " << hint << "\n";
    }

    return ss.str();
}

void Diagnostics::report_lexer_error(const LexerError& error, std::string_view source_code, std::string_view filename) {
    SourceSpan span{error.location, error.location};
    std::cerr << format_error("LexerError", error.message, span, source_code, filename);
}

void Diagnostics::report_parser_error(const ParserError& error, std::string_view source_code, std::string_view filename) {
    std::cerr << format_error(error.category.empty() ? "SyntaxError" : error.category, error.message, error.span, source_code, filename, error.hint);
}

void Diagnostics::report_runtime_error(const NovaRuntimeError& error, std::string_view source_code, std::string_view filename) {
    std::cerr << format_error(
        NovaRuntimeError::error_type_name(error.error_type()),
        error.message(),
        error.span(),
        source_code,
        filename,
        error.hint()
    );
}

}  // namespace nova
