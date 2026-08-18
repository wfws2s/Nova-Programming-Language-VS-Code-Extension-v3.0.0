#pragma once

#include "lexer/lexer_error.hpp"
#include "parser/parser_error.hpp"
#include "runtime/runtime_error.hpp"
#include "source/source_location.hpp"

#include <string>
#include <string_view>

namespace nova {

class Diagnostics {
public:
    static void report_lexer_error(const LexerError& error, std::string_view source_code, std::string_view filename = "<stdin>");
    static void report_parser_error(const ParserError& error, std::string_view source_code, std::string_view filename = "<stdin>");
    static void report_runtime_error(const NovaRuntimeError& error, std::string_view source_code, std::string_view filename = "<stdin>");

    static std::string format_error(
        std::string_view category,
        std::string_view message,
        SourceSpan span,
        std::string_view source_code,
        std::string_view filename = "<stdin>",
        std::string_view hint = ""
    );
};

}  // namespace nova
