#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

#include <cstdlib>
#include <cmath>
#include <iostream>

namespace nova {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    // A parser can be constructed by embedding code with an empty token list.
    // Keep navigation safe and report an empty program rather than dereferencing
    // tokens_.front()/back().
    if (tokens_.empty()) {
        tokens_.push_back(Token{TokenType::Eof, "", SourceSpan{}});
    }
}

const Token& Parser::peek() const {
    if (current_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_];
}

const Token& Parser::previous() const {
    if (current_ == 0) {
        return tokens_.front();
    }
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::Eof;
}

const Token& Parser::advance() {
    if (!is_at_end()) {
        ++current_;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) {
        return type == TokenType::Eof;
    }
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_any(std::initializer_list<TokenType> types) {
    for (TokenType t : types) {
        if (check(t)) {
            advance();
            return true;
        }
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    report_error(peek(), message);
    return peek();
}

const Token& Parser::consume_binding_name(const std::string& description) {
    if (check(TokenType::Identifier)) {
        return advance();
    }
    if (peek().is_keyword()) {
        report_reserved_name_error(peek(), "cannot use reserved word '" + peek().lexeme + "' as a variable name");
        return advance();
    }
    report_error(peek(), "Expected " + description);
    return peek();
}

void Parser::skip_newlines() {
    while (check(TokenType::Newline)) {
        advance();
    }
}

void Parser::consume_statement_terminator() {
    if (check(TokenType::Newline)) {
        advance();
        skip_newlines();
    } else if (check(TokenType::Eof) || check(TokenType::End) || check(TokenType::Else) ||
               check(TokenType::Elif) || check(TokenType::Catch)) {
        // Naturally terminated by block boundary or EOF
    } else {
        report_error(peek(), "Expected newline or end of statement");
    }
}

void Parser::report_error(const Token& token, std::string message) {
    errors_.push_back(ParserError{std::move(message), token.span.start, token.span, "SyntaxError"});
}

void Parser::report_reserved_name_error(const Token& token, const std::string& message) {
    std::string msg = message.empty() ? ("cannot use reserved word '" + token.lexeme + "' as a variable name") : message;
    errors_.push_back(ParserError{std::move(msg), token.span.start, token.span, "ReservedNameError", "'" + token.lexeme + "' is a reserved keyword in NOVA."});
}

void Parser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous().type == TokenType::Newline) {
            return;
        }
        switch (peek().type) {
            case TokenType::Let:
            case TokenType::If:
            case TokenType::While:
            case TokenType::For:
            case TokenType::Fn:
            case TokenType::Return:
            case TokenType::Try:
            case TokenType::Import:
            case TokenType::End:
                return;
            default:
                break;
        }
        advance();
    }
}

Parser::Precedence Parser::get_precedence(TokenType type) {
    switch (type) {
        case TokenType::Equal:
            return Precedence::Assignment;
        case TokenType::Or:
            return Precedence::Or;
        case TokenType::And:
            return Precedence::And;
        case TokenType::EqualEqual:
        case TokenType::BangEqual:
        case TokenType::Is:
            return Precedence::Equality;
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            return Precedence::Comparison;
        case TokenType::DotDot:
            return Precedence::Range;
        case TokenType::Plus:
        case TokenType::Minus:
            return Precedence::Term;
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            return Precedence::Factor;
        case TokenType::StarStar:
            return Precedence::Power;
        case TokenType::Dot:
        case TokenType::LeftParen:
        case TokenType::LeftBracket:
            return Precedence::Call;
        default:
            return Precedence::None;
    }
}

std::unique_ptr<ast::Program> Parser::parse_program() {
    std::vector<std::unique_ptr<ast::Stmt>> statements;
    skip_newlines();

    while (!is_at_end()) {
        if (auto stmt = parse_declaration_or_statement()) {
            statements.push_back(std::move(stmt));
        }
        skip_newlines();
    }

    return std::make_unique<ast::Program>(std::move(statements));
}

std::unique_ptr<ast::Stmt> Parser::parse_declaration_or_statement() {
    try {
        if (peek().is_keyword() && peek().type != TokenType::Let && peek().type != TokenType::If &&
            peek().type != TokenType::While && peek().type != TokenType::For && peek().type != TokenType::Fn &&
            peek().type != TokenType::Return && peek().type != TokenType::Try && peek().type != TokenType::Import &&
            peek().type != TokenType::Class && peek().type != TokenType::Global) {
            if (current_ + 1 < tokens_.size() && tokens_[current_ + 1].type == TokenType::Equal) {
                report_reserved_name_error(advance());
                if (match(TokenType::Equal)) {
                    parse_expression();
                }
                consume_statement_terminator();
                return nullptr;
            }
        }

        if (match(TokenType::Let)) {
            return parse_let_statement();
        }
        if (match(TokenType::If)) {
            return parse_if_statement();
        }
        if (match(TokenType::While)) {
            return parse_while_statement();
        }
        if (match(TokenType::For)) {
            return parse_for_statement();
        }
        if (match(TokenType::Async)) {
            // async fn ...
            if (match(TokenType::Fn)) {
                return parse_fn_declaration(true);
            }
            report_error(peek(), "Expected 'fn' after 'async'");
            synchronize();
            return nullptr;
        }
        if (match(TokenType::Fn)) {
            return parse_fn_declaration();
        }
        if (match(TokenType::Return)) {
            return parse_return_statement();
        }
        if (match(TokenType::Try)) {
            return parse_try_statement();
        }
        if (match(TokenType::Import)) {
            return parse_import_statement();
        }
        if (match(TokenType::Class)) {
            return parse_class_declaration();
        }
        if (match(TokenType::Global)) {
            return parse_global_statement();
        }
        return parse_expression_statement();
    } catch (...) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<ast::Stmt> Parser::parse_let_statement() {
    Token let_token = previous();
    const Token& name_token = consume_binding_name("variable name after 'let'");
    std::string name = name_token.lexeme;

    std::unique_ptr<ast::Expr> initializer = nullptr;
    if (match(TokenType::Equal)) {
        initializer = parse_expression();
    } else {
        // Default initializer is null
        initializer = ast::LiteralExpr::make_null(name_token.span);
    }

    SourceSpan span{let_token.span.start, initializer ? initializer->span.end : name_token.span.end};
    consume_statement_terminator();
    return std::make_unique<ast::LetStmt>(std::move(name), std::move(initializer), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_if_statement() {
    Token if_token = previous();
    auto condition = parse_expression();
    consume_statement_terminator();

    auto then_branch = parse_block();
    std::vector<ast::ElifBranch> elif_branches;

    while (match(TokenType::Elif)) {
        Token elif_token = previous();
        auto elif_cond = parse_expression();
        consume_statement_terminator();
        auto elif_body = parse_block();
        SourceSpan elif_span{elif_token.span.start, previous().span.end};
        elif_branches.push_back(ast::ElifBranch{std::move(elif_cond), std::move(elif_body), elif_span});
    }

    std::unique_ptr<ast::BlockStmt> else_branch = nullptr;
    if (match(TokenType::Else)) {
        consume_statement_terminator();
        else_branch = parse_block();
    }

    const Token& end_token = consume(TokenType::End, "Expected 'end' after if statement");
    SourceSpan span{if_token.span.start, end_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::IfStmt>(
        std::move(condition), std::move(then_branch), std::move(elif_branches), std::move(else_branch), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_while_statement() {
    Token while_token = previous();
    auto condition = parse_expression();
    consume_statement_terminator();

    auto body = parse_block();

    const Token& end_token = consume(TokenType::End, "Expected 'end' after while body");
    SourceSpan span{while_token.span.start, end_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::WhileStmt>(std::move(condition), std::move(body), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_for_statement() {
    Token for_token = previous();
    const Token& var_token = consume_binding_name("loop variable name after 'for'");
    std::string var_name = var_token.lexeme;

    consume(TokenType::In, "Expected 'in' after for loop variable");
    auto iterable = parse_expression();
    consume_statement_terminator();

    auto body = parse_block();

    const Token& end_token = consume(TokenType::End, "Expected 'end' after for body");
    SourceSpan span{for_token.span.start, end_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::ForStmt>(std::move(var_name), std::move(iterable), std::move(body), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_fn_declaration(bool is_async) {
    Token fn_token = previous();
    const Token& name_token = consume_binding_name("function name after 'fn'");
    std::string name = name_token.lexeme;

    consume(TokenType::LeftParen, "Expected '(' after function name");
    std::vector<std::string> params;

    if (!check(TokenType::RightParen)) {
        do {
            const Token& param_token = consume_binding_name("parameter name");
            params.push_back(param_token.lexeme);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expected ')' after function parameters");
    consume_statement_terminator();

    auto body = parse_block();

    const Token& end_token = consume(TokenType::End, "Expected 'end' after function body");
    SourceSpan span{fn_token.span.start, end_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::FnDeclStmt>(
        std::move(name), std::move(params), std::move(body), is_async, span);
}

std::unique_ptr<ast::Stmt> Parser::parse_return_statement() {
    Token ret_token = previous();
    std::unique_ptr<ast::Expr> value = nullptr;

    if (!check(TokenType::Newline) && !check(TokenType::Eof) && !check(TokenType::End) && !check(TokenType::Else) &&
        !check(TokenType::Elif) && !check(TokenType::Catch)) {
        value = parse_expression();
    }

    SourceSpan span{ret_token.span.start, value ? value->span.end : ret_token.span.end};
    consume_statement_terminator();
    return std::make_unique<ast::ReturnStmt>(std::move(value), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_try_statement() {
    Token try_token = previous();
    consume_statement_terminator();

    auto try_branch = parse_block();

    consume(TokenType::Catch, "Expected 'catch' after try block");
    const Token& err_var_token = consume_binding_name("variable name after 'catch'");
    std::string err_var = err_var_token.lexeme;
    consume_statement_terminator();

    auto catch_branch = parse_block();

    const Token& end_token = consume(TokenType::End, "Expected 'end' after try-catch block");
    SourceSpan span{try_token.span.start, end_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::TryCatchStmt>(
        std::move(try_branch), std::move(err_var), std::move(catch_branch), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_import_statement() {
    Token import_token = previous();
    std::string module_name;
    std::string alias;
    bool is_path = false;

    if (match(TokenType::String)) {
        module_name = previous().lexeme;
        is_path = true;
        if (match(TokenType::As)) {
            const Token& alias_token = consume_binding_name("alias name after 'as'");
            alias = alias_token.lexeme;
        } else {
            // Default alias: extract file stem
            std::string p = module_name;
            auto last_slash = p.find_last_of("/\\");
            std::string fname = (last_slash == std::string::npos) ? p : p.substr(last_slash + 1);
            auto dot_pos = fname.rfind('.');
            alias = (dot_pos == std::string::npos) ? fname : fname.substr(0, dot_pos);
        }
    } else if (check(TokenType::Identifier)) {
        module_name = advance().lexeme;
        alias = module_name;
        if (match(TokenType::As)) {
            const Token& alias_token = consume_binding_name("alias name after 'as'");
            alias = alias_token.lexeme;
        }
    } else {
        report_error(peek(), "Expected module name or file path string after 'import'");
    }

    SourceSpan span{import_token.span.start, previous().span.end};
    consume_statement_terminator();
    return std::make_unique<ast::ImportStmt>(std::move(module_name), std::move(alias), is_path, span);
}

std::unique_ptr<ast::Stmt> Parser::parse_class_declaration() {
    Token class_token = previous();
    const Token& name_token = consume_binding_name("class name after 'class'");
    std::string name = name_token.lexeme;
    std::string parent_name = "";

    if (match(TokenType::Extends) || match(TokenType::Colon)) {
        const Token& parent_token = consume_binding_name("parent class name");
        parent_name = parent_token.lexeme;
    } else if (match(TokenType::LeftParen)) {
        if (!check(TokenType::RightParen)) {
            const Token& parent_token = consume_binding_name("parent class name");
            parent_name = parent_token.lexeme;
        }
        consume(TokenType::RightParen, "Expected ')' after parent class name");
    }

    consume_statement_terminator();
    skip_newlines();

    std::vector<ast::MethodDecl> methods;
    while (!check(TokenType::End) && !is_at_end()) {
        bool is_static = false;
        if (match(TokenType::Static)) {
            is_static = true;
            match(TokenType::Fn);
        } else {
            consume(TokenType::Fn, "Expected 'fn' or 'static fn' for method declaration in class");
        }

        Token fn_token = previous();
        const Token& m_name_token = consume_binding_name("method name");
        std::string m_name = m_name_token.lexeme;

        consume(TokenType::LeftParen, "Expected '(' after method name");
        std::vector<std::string> params;
        if (!check(TokenType::RightParen)) {
            do {
                if (check(TokenType::RightParen)) break;
                if (check(TokenType::Self)) {
                    advance();
                    params.push_back("self");
                } else {
                    const Token& param_token = consume_binding_name("parameter name");
                    params.push_back(param_token.lexeme);
                }
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "Expected ')' after method parameters");
        consume_statement_terminator();

        auto body = parse_block();
        const Token& end_tok = consume(TokenType::End, "Expected 'end' after method body");
        SourceSpan m_span{fn_token.span.start, end_tok.span.end};
        consume_statement_terminator();

        methods.push_back(ast::MethodDecl{
            std::move(m_name),
            std::move(params),
            std::move(body),
            is_static,
            m_span
        });
        skip_newlines();
    }

    const Token& end_class_token = consume(TokenType::End, "Expected 'end' after class body");
    SourceSpan span{class_token.span.start, end_class_token.span.end};
    consume_statement_terminator();

    return std::make_unique<ast::ClassDeclStmt>(
        std::move(name), std::move(parent_name), std::move(methods), span);
}

std::unique_ptr<ast::Stmt> Parser::parse_global_statement() {
    Token global_tok = previous();
    const Token& var_token = consume_binding_name("variable name after 'global'");
    SourceSpan span{global_tok.span.start, var_token.span.end};
    consume_statement_terminator();
    return std::make_unique<ast::GlobalStmt>(var_token.lexeme, span);
}

std::unique_ptr<ast::Stmt> Parser::parse_expression_statement() {
    auto expr = parse_expression();
    if (!expr) {
        return nullptr;
    }
    SourceSpan span = expr->span;
    consume_statement_terminator();
    return std::make_unique<ast::ExprStmt>(std::move(expr), span);
}

std::unique_ptr<ast::BlockStmt> Parser::parse_block() {
    std::vector<std::unique_ptr<ast::Stmt>> stmts;
    skip_newlines();

    while (!check(TokenType::End) && !check(TokenType::Else) && !check(TokenType::Elif) &&
           !check(TokenType::Catch) && !is_at_end()) {
        if (auto stmt = parse_declaration_or_statement()) {
            stmts.push_back(std::move(stmt));
        }
        skip_newlines();
    }

    return std::make_unique<ast::BlockStmt>(std::move(stmts), SourceSpan{});
}

// Pratt Parser Implementations
std::unique_ptr<ast::Expr> Parser::parse_expression(Precedence precedence) {
    skip_newlines();
    auto left = parse_prefix();
    if (!left) {
        return nullptr;
    }

    while (precedence < get_precedence(peek().type)) {
        left = parse_infix(std::move(left));
        if (!left) {
            break;
        }
    }

    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_prefix() {
    if (match(TokenType::Number)) {
        return parse_number_literal();
    }
    if (match(TokenType::String)) {
        return parse_string_literal();
    }
    if (match(TokenType::True)) {
        return ast::LiteralExpr::make_bool(true, previous().span);
    }
    if (match(TokenType::False)) {
        return ast::LiteralExpr::make_bool(false, previous().span);
    }
    if (match(TokenType::Null)) {
        return ast::LiteralExpr::make_null(previous().span);
    }
    if (match(TokenType::Self)) {
        return std::make_unique<ast::VariableExpr>("self", previous().span);
    }
    if (match(TokenType::Super)) {
        Token super_tok = previous();
        std::string method_name = "";
        if (match(TokenType::Dot)) {
            const Token& m_token = consume(TokenType::Identifier, "Expected method name after 'super.'");
            method_name = m_token.lexeme;
        }
        consume(TokenType::LeftParen, "Expected '(' after super or super.method");
        std::vector<std::unique_ptr<ast::Expr>> args;
        skip_newlines();
        if (!check(TokenType::RightParen)) {
            do {
                skip_newlines();
                if (check(TokenType::RightParen)) break;
                args.push_back(parse_expression());
                skip_newlines();
            } while (match(TokenType::Comma));
        }
        const Token& close_p = consume(TokenType::RightParen, "Expected ')' after super arguments");
        SourceSpan span{super_tok.span.start, close_p.span.end};
        return std::make_unique<ast::SuperCallExpr>(method_name, std::move(args), span);
    }
    if (match(TokenType::Type)) {
        Token type_tok = previous();
        auto expr = parse_expression(Precedence::Unary);
        SourceSpan span{type_tok.span.start, expr ? expr->span.end : type_tok.span.end};
        return std::make_unique<ast::TypeExpr>(std::move(expr), span);
    }
    if (match(TokenType::New)) {
        Token new_tok = previous();
        const Token& cls_tok = consume_binding_name("class name after 'new'");
        std::string cls_name = cls_tok.lexeme;
        consume(TokenType::LeftParen, "Expected '(' after class name");
        std::vector<std::unique_ptr<ast::Expr>> args;
        skip_newlines();
        if (!check(TokenType::RightParen)) {
            do {
                skip_newlines();
                if (check(TokenType::RightParen)) break;
                args.push_back(parse_expression());
                skip_newlines();
            } while (match(TokenType::Comma));
        }
        const Token& close_p = consume(TokenType::RightParen, "Expected ')' after arguments");
        SourceSpan span{new_tok.span.start, close_p.span.end};
        return std::make_unique<ast::NewExpr>(std::move(cls_name), std::move(args), span);
    }
    if (match(TokenType::Await)) {
        Token op = previous();
        auto right = parse_expression(Precedence::Unary);
        SourceSpan span{op.span.start, right ? right->span.end : op.span.end};
        return std::make_unique<ast::UnaryExpr>(op, std::move(right), span);
    }
    if (match(TokenType::Identifier)) {
        Token id_token = previous();
        return std::make_unique<ast::VariableExpr>(id_token.lexeme, id_token.span);
    }
    if (match(TokenType::LeftParen)) {
        return parse_grouping();
    }
    if (match(TokenType::LeftBracket)) {
        return parse_array();
    }
    if (match(TokenType::LeftBrace)) {
        return parse_dictionary();
    }
    if (match(TokenType::Minus) || match(TokenType::Not)) {
        Token op = previous();
        auto right = parse_expression(Precedence::Unary);
        SourceSpan span{op.span.start, right ? right->span.end : op.span.end};
        return std::make_unique<ast::UnaryExpr>(op, std::move(right), span);
    }

    report_error(peek(), "Expected expression, got '" + peek().lexeme + "'");
    advance();
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_infix(std::unique_ptr<ast::Expr> left) {
    TokenType type = peek().type;

    if (type == TokenType::Equal) {
        advance(); // consume '='
        auto right = parse_expression(Precedence::Assignment);
        if (auto* var_expr = dynamic_cast<ast::VariableExpr*>(left.get())) {
            SourceSpan span{left->span.start, right ? right->span.end : previous().span.end};
            return std::make_unique<ast::AssignExpr>(var_expr->name, std::move(right), span);
        }
        if (auto* idx_expr = dynamic_cast<ast::IndexExpr*>(left.get())) {
            SourceSpan span{left->span.start, right ? right->span.end : previous().span.end};
            return std::make_unique<ast::IndexAssignExpr>(
                std::move(idx_expr->target), std::move(idx_expr->index), std::move(right), span);
        }
        if (auto* mem_expr = dynamic_cast<ast::MemberExpr*>(left.get())) {
            SourceSpan span{left->span.start, right ? right->span.end : previous().span.end};
            return std::make_unique<ast::MemberAssignExpr>(
                std::move(mem_expr->target), mem_expr->member, std::move(right), span);
        }
        report_error(previous(), "Invalid assignment target");
        return nullptr;
    }

    if (type == TokenType::Dot) {
        advance(); // consume '.'
        const Token& member_token = consume(TokenType::Identifier, "Expected property name after '.'");
        SourceSpan span{left->span.start, member_token.span.end};
        return std::make_unique<ast::MemberExpr>(std::move(left), member_token.lexeme, span);
    }

    if (type == TokenType::Is) {
        Token op = advance(); // consume 'is'
        auto right = parse_expression(Precedence::Equality);
        SourceSpan span{left->span.start, right ? right->span.end : op.span.end};
        return std::make_unique<ast::BinaryExpr>(std::move(left), op, std::move(right), span);
    }

    if (type == TokenType::LeftParen) {
        return parse_call(std::move(left));
    }

    if (type == TokenType::LeftBracket) {
        return parse_index(std::move(left));
    }

    if (type == TokenType::DotDot) {
        advance(); // consume '..'
        std::unique_ptr<ast::Expr> right = nullptr;
        if (!check(TokenType::RightBracket)) {
            right = parse_expression(Precedence::Range);
        }
        SourceSpan span{left->span.start, right ? right->span.end : previous().span.end};
        return std::make_unique<ast::RangeExpr>(std::move(left), std::move(right), span);
    }

    if (type == TokenType::StarStar) {
        Token op = advance(); // consume '**'
        // Right-associative: parse right with Precedence::Factor so 2 ** 3 ** 2 is 2 ** (3 ** 2)
        auto right = parse_expression(Precedence::Factor);
        SourceSpan span{left->span.start, right ? right->span.end : op.span.end};
        return std::make_unique<ast::BinaryExpr>(std::move(left), op, std::move(right), span);
    }

    Token op = advance();
    Precedence prec = get_precedence(op.type);
    auto right = parse_expression(prec);
    SourceSpan span{left->span.start, right ? right->span.end : op.span.end};
    return std::make_unique<ast::BinaryExpr>(std::move(left), op, std::move(right), span);
}

std::unique_ptr<ast::Expr> Parser::parse_number_literal() {
    Token token = previous();
    try {
        const double value = std::stod(token.lexeme);
        if (!std::isfinite(value)) {
            report_error(token, "Invalid number literal '" + token.lexeme + "'");
            return nullptr;
        }
        return ast::LiteralExpr::make_number(value, token.span);
    } catch (const std::exception&) {
        report_error(token, "Invalid number literal '" + token.lexeme + "'");
        return nullptr;
    }
}

std::unique_ptr<ast::Expr> Parser::parse_string_literal() {
    Token token = previous();
    const std::string& raw = token.lexeme;

    std::vector<std::unique_ptr<ast::Expr>> parts;
    std::string current_text;
    bool has_interpolation = false;
    std::size_t i = 0;

    while (i < raw.size()) {
        if (raw[i] == '{') {
            if (i + 1 < raw.size() && raw[i + 1] == '{') {
                current_text.push_back('{');
                i += 2;
                continue;
            }
            has_interpolation = true;
            if (!current_text.empty()) {
                parts.push_back(ast::LiteralExpr::make_string(current_text, token.span));
                current_text.clear();
            }
            ++i; // skip '{'
            std::size_t expr_start = i;
            int brace_depth = 1;
            bool in_str = false;
            char str_quote = '\0';

            while (i < raw.size() && brace_depth > 0) {
                char c = raw[i];
                if (in_str) {
                    if (c == '\\' && i + 1 < raw.size()) {
                        i += 2;
                        continue;
                    }
                    if (c == str_quote) {
                        in_str = false;
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        in_str = true;
                        str_quote = c;
                    } else if (c == '{') {
                        ++brace_depth;
                    } else if (c == '}') {
                        --brace_depth;
                    }
                }
                if (brace_depth > 0) {
                    ++i;
                }
            }

            if (brace_depth > 0) {
                report_error(token, "Unterminated expression in string interpolation");
                return nullptr;
            }

            std::string expr_text = raw.substr(expr_start, i - expr_start);
            ++i; // skip '}'

            Lexer sub_lexer(expr_text);
            auto sub_tokens = sub_lexer.tokenize();
            if (sub_lexer.had_error()) {
                report_error(token, "Lexer error in string interpolation: " + sub_lexer.error().message);
                return nullptr;
            }
            Parser sub_parser(std::move(sub_tokens));
            auto sub_expr = sub_parser.parse_expression();
            if (sub_parser.had_error() || !sub_expr) {
                report_error(token, "Syntax error in string interpolation expression");
                return nullptr;
            }
            parts.push_back(std::move(sub_expr));
        } else if (raw[i] == '}') {
            if (i + 1 < raw.size() && raw[i + 1] == '}') {
                current_text.push_back('}');
                i += 2;
                continue;
            }
            current_text.push_back('}');
            ++i;
        } else {
            current_text.push_back(raw[i]);
            ++i;
        }
    }

    if (!current_text.empty() || parts.empty()) {
        parts.push_back(ast::LiteralExpr::make_string(current_text, token.span));
    }

    if (!has_interpolation && parts.size() == 1) {
        return std::move(parts[0]);
    }

    return std::make_unique<ast::InterpolatedStringExpr>(std::move(parts), token.span);
}

std::unique_ptr<ast::Expr> Parser::parse_grouping() {
    Token open_paren = previous();
    auto expr = parse_expression();
    const Token& close_paren = consume(TokenType::RightParen, "Expected ')' after grouping expression");
    SourceSpan span{open_paren.span.start, close_paren.span.end};
    return std::make_unique<ast::GroupingExpr>(std::move(expr), span);
}

std::unique_ptr<ast::Expr> Parser::parse_array() {
    Token open_bracket = previous();
    std::vector<std::unique_ptr<ast::Expr>> elements;

    skip_newlines();
    if (!check(TokenType::RightBracket)) {
        do {
            skip_newlines();
            if (check(TokenType::RightBracket)) break;
            elements.push_back(parse_expression());
            skip_newlines();
        } while (match(TokenType::Comma));
    }

    const Token& close_bracket = consume(TokenType::RightBracket, "Expected ']' after array elements");
    SourceSpan span{open_bracket.span.start, close_bracket.span.end};
    return std::make_unique<ast::ListExpr>(std::move(elements), span);
}

std::unique_ptr<ast::Expr> Parser::parse_dictionary() {
    Token open_brace = previous();
    std::vector<ast::DictExpr::Entry> entries;

    skip_newlines();
    if (!check(TokenType::RightBrace)) {
        do {
            skip_newlines();
            if (check(TokenType::RightBrace)) break;
            auto key = parse_expression();
            consume(TokenType::Colon, "Expected ':' after dictionary key");
            auto val = parse_expression();
            entries.push_back(ast::DictExpr::Entry{std::move(key), std::move(val)});
            skip_newlines();
        } while (match(TokenType::Comma));
    }

    const Token& close_brace = consume(TokenType::RightBrace, "Expected '}' after dictionary entries");
    SourceSpan span{open_brace.span.start, close_brace.span.end};
    return std::make_unique<ast::DictExpr>(std::move(entries), span);
}

std::unique_ptr<ast::Expr> Parser::parse_call(std::unique_ptr<ast::Expr> callee) {
    advance(); // consume '('
    std::vector<std::unique_ptr<ast::Expr>> arguments;

    skip_newlines();
    if (!check(TokenType::RightParen)) {
        do {
            skip_newlines();
            if (check(TokenType::RightParen)) break;
            arguments.push_back(parse_expression());
            skip_newlines();
        } while (match(TokenType::Comma));
    }

    const Token& close_paren = consume(TokenType::RightParen, "Expected ')' after arguments");
    SourceSpan span{callee->span.start, close_paren.span.end};
    return std::make_unique<ast::CallExpr>(std::move(callee), std::move(arguments), span);
}

std::unique_ptr<ast::Expr> Parser::parse_index(std::unique_ptr<ast::Expr> target) {
    Token open_bracket = advance(); // consume '['
    skip_newlines();

    if (match(TokenType::DotDot)) {
        // [..end] or [..]
        skip_newlines();
        std::unique_ptr<ast::Expr> end = nullptr;
        if (!check(TokenType::RightBracket)) {
            end = parse_expression();
            skip_newlines();
        }
        const Token& close_bracket = consume(TokenType::RightBracket, "Expected ']' after slice");
        SourceSpan span{target->span.start, close_bracket.span.end};
        return std::make_unique<ast::SliceExpr>(std::move(target), nullptr, std::move(end), span);
    }

    if (check(TokenType::RightBracket)) {
        report_error(peek(), "Expected index or slice expression inside '[]'");
        const Token& close_bracket = advance();
        SourceSpan span{target->span.start, close_bracket.span.end};
        return std::make_unique<ast::IndexExpr>(std::move(target), nullptr, span);
    }

    auto expr = parse_expression();
    skip_newlines();

    if (auto* range_expr = dynamic_cast<ast::RangeExpr*>(expr.get())) {
        const Token& close_bracket = consume(TokenType::RightBracket, "Expected ']' after slice");
        SourceSpan span{target->span.start, close_bracket.span.end};
        return std::make_unique<ast::SliceExpr>(std::move(target), std::move(range_expr->start), std::move(range_expr->end), span);
    }

    if (match(TokenType::DotDot)) {
        std::unique_ptr<ast::Expr> end = nullptr;
        if (!check(TokenType::RightBracket)) {
            end = parse_expression();
            skip_newlines();
        }
        const Token& close_bracket = consume(TokenType::RightBracket, "Expected ']' after slice");
        SourceSpan span{target->span.start, close_bracket.span.end};
        return std::make_unique<ast::SliceExpr>(std::move(target), std::move(expr), std::move(end), span);
    }

    const Token& close_bracket = consume(TokenType::RightBracket, "Expected ']' after index");
    SourceSpan span{target->span.start, close_bracket.span.end};
    return std::make_unique<ast::IndexExpr>(std::move(target), std::move(expr), span);
}

}  // namespace nova
