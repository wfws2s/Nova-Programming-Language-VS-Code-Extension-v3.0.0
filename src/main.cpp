#include "interpreter/diagnostics.hpp"
#include "interpreter/interpreter.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "version.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace nova {

namespace {

void print_usage() {
    std::cout << "nova " << NOVA_VERSION_STRING << " — NOVA programming language interpreter\n"
              << "\n"
              << "Usage:\n"
              << "  nova --version          Show version information\n"
              << "  nova --help             Show this help message\n"
              << "  nova <file>             Run a NOVA source file\n"
              << "  nova                    Start interactive REPL\n"
              << "\n";
}

void print_version() {
    std::cout << "nova " << NOVA_VERSION_STRING << "\n";
}

int run_source(const std::string& source, const std::string& filename = "<stdin>", Interpreter* custom_interp = nullptr) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    if (lexer.had_error()) {
        Diagnostics::report_lexer_error(lexer.error(), source, filename);
        return 1;
    }

    Parser parser(std::move(tokens));
    auto program = parser.parse_program();

    if (parser.had_error()) {
        for (const auto& err : parser.errors()) {
            Diagnostics::report_parser_error(err, source, filename);
        }
        return 1;
    }

    try {
        if (custom_interp) {
            custom_interp->set_current_file(filename);
            custom_interp->interpret(*program);
        } else {
            Interpreter interpreter;
            interpreter.set_current_file(filename);
            interpreter.interpret(*program);
        }
    } catch (const NovaRuntimeError& err) {
        Diagnostics::report_runtime_error(err, source, filename);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Internal Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

int run_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: file not found: " << path.string() << "\n";
        return 1;
    }

    std::ifstream input(path);
    if (!input) {
        std::cerr << "Error: could not open file: " << path.string() << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    return run_source(buffer.str(), path.string());
}

void run_repl() {
    std::cout << "NOVA " << NOVA_VERSION_STRING << " Interactive REPL\n"
              << "Type 'exit' or press Ctrl+C to quit.\n\n";

    Interpreter interpreter;
    std::string line;

    while (true) {
        std::cout << "nova> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        run_source(line + "\n", "<repl>", &interpreter);
    }
}

}  // namespace

}  // namespace nova

int main(int argc, char* argv[]) {
    if (argc < 2) {
        nova::run_repl();
        return 0;
    }

    const std::string arg = argv[1];

    if (arg == "--version" || arg == "-v") {
        nova::print_version();
        return 0;
    }

    if (arg == "--help" || arg == "-h") {
        nova::print_usage();
        return 0;
    }

    return nova::run_file(argv[1]);
}
