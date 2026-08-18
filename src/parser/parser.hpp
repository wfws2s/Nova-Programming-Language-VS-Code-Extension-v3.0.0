#pragma once

#include "ast/ast.hpp"
#include "lexer/token.hpp"
#include "parser/parser_error.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nova {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<ast::Program> parse_program();
    bool had_error() const { return !errors_.empty(); }
    const std::vector<ParserError>& errors() const { return errors_; }

private:
    enum class Precedence {
        None,
        Assignment,   // =
        Or,           // or
        And,          // and
        Equality,     // == !=
        Comparison,   // < <= > >=
        Range,        // ..
        Term,         // + -
        Factor,       // * / %
        Power,        // **
        Unary,        // not, -
        Call,         // () []
        Primary
    };

    // Token navigation
    const Token& peek() const;
    const Token& previous() const;
    bool is_at_end() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match_any(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& message);
    const Token& consume_binding_name(const std::string& description);
    void skip_newlines();
    void consume_statement_terminator();

    // Errors
    void report_error(const Token& token, std::string message);
    void report_reserved_name_error(const Token& token, const std::string& message = "");
    void synchronize();

    // Precedence mapping
    static Precedence get_precedence(TokenType type);

    // Expressions (Pratt Parser)
    std::unique_ptr<ast::Expr> parse_expression(Precedence precedence = Precedence::None);
    std::unique_ptr<ast::Expr> parse_prefix();
    std::unique_ptr<ast::Expr> parse_infix(std::unique_ptr<ast::Expr> left);

    // Specific expressions
    std::unique_ptr<ast::Expr> parse_number_literal();
    std::unique_ptr<ast::Expr> parse_string_literal();
    std::unique_ptr<ast::Expr> parse_grouping();
    std::unique_ptr<ast::Expr> parse_array();
    std::unique_ptr<ast::Expr> parse_dictionary();
    std::unique_ptr<ast::Expr> parse_call(std::unique_ptr<ast::Expr> callee);
    std::unique_ptr<ast::Expr> parse_index(std::unique_ptr<ast::Expr> target);
    std::unique_ptr<ast::Expr> parse_member(std::unique_ptr<ast::Expr> target);

    // Statements
    std::unique_ptr<ast::Stmt> parse_declaration_or_statement();
    std::unique_ptr<ast::Stmt> parse_let_statement();
    std::unique_ptr<ast::Stmt> parse_if_statement();
    std::unique_ptr<ast::Stmt> parse_while_statement();
    std::unique_ptr<ast::Stmt> parse_for_statement();
    std::unique_ptr<ast::Stmt> parse_fn_declaration();
    std::unique_ptr<ast::Stmt> parse_return_statement();
    std::unique_ptr<ast::Stmt> parse_try_statement();
    std::unique_ptr<ast::Stmt> parse_import_statement();
    std::unique_ptr<ast::Stmt> parse_class_declaration();
    std::unique_ptr<ast::Stmt> parse_global_statement();
    std::unique_ptr<ast::Stmt> parse_expression_statement();
    std::unique_ptr<ast::BlockStmt> parse_block();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::vector<ParserError> errors_;
};

}  // namespace nova
