#pragma once

#include "ast/ast.hpp"
#include "runtime/environment.hpp"
#include "runtime/runtime_error.hpp"
#include "runtime/value.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova {

struct CallFrame {
    std::string function_name;
    std::string filename;
    SourceSpan call_span;
};

class Interpreter : public ast::ASTVisitor {
public:
    explicit Interpreter(std::ostream& output_stream = std::cout,
                         std::istream& input_stream = std::cin);

    void set_current_file(std::string filename) {
        current_filename_ = std::move(filename);
    }
    const std::string& current_file() const { return current_filename_; }

    void interpret(ast::Program& program);
    Value evaluate(ast::Expr& expr);
    void execute(ast::Stmt& stmt);
    void execute_block(const std::vector<std::unique_ptr<ast::Stmt>>& statements,
                       std::shared_ptr<Environment> environment);

    std::shared_ptr<Environment> global_env() const { return globals_; }
    std::shared_ptr<Environment> current_env() const { return environment_; }

    std::string format_stack_trace(const SourceSpan& failing_span) const;

    // AST Visitor overrides
    void visit(ast::LiteralExpr& expr) override;
    void visit(ast::VariableExpr& expr) override;
    void visit(ast::AssignExpr& expr) override;
    void visit(ast::BinaryExpr& expr) override;
    void visit(ast::UnaryExpr& expr) override;
    void visit(ast::GroupingExpr& expr) override;
    void visit(ast::CallExpr& expr) override;
    void visit(ast::ListExpr& expr) override;
    void visit(ast::DictExpr& expr) override;
    void visit(ast::RangeExpr& expr) override;
    void visit(ast::IndexExpr& expr) override;
    void visit(ast::IndexAssignExpr& expr) override;
    void visit(ast::SliceExpr& expr) override;
    void visit(ast::MemberExpr& expr) override;
    void visit(ast::MemberAssignExpr& expr) override;
    void visit(ast::InterpolatedStringExpr& expr) override;
    void visit(ast::NewExpr& expr) override;
    void visit(ast::TypeExpr& expr) override;
    void visit(ast::SuperCallExpr& expr) override;

    void visit(ast::ExprStmt& stmt) override;
    void visit(ast::LetStmt& stmt) override;
    void visit(ast::BlockStmt& stmt) override;
    void visit(ast::IfStmt& stmt) override;
    void visit(ast::WhileStmt& stmt) override;
    void visit(ast::ForStmt& stmt) override;
    void visit(ast::FnDeclStmt& stmt) override;
    void visit(ast::ReturnStmt& stmt) override;
    void visit(ast::TryCatchStmt& stmt) override;
    void visit(ast::ImportStmt& stmt) override;
    void visit(ast::GlobalStmt& stmt) override;
    void visit(ast::ClassDeclStmt& stmt) override;
    void visit(ast::Program& program) override;

private:
    struct ReturnSignal {
        Value value;
    };

    std::shared_ptr<DictObject> get_or_create_std_module(const std::string& name);

    Value last_value_;
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
    std::ostream& out_;
    std::istream& in_;

    std::string current_filename_ = "<main>";
    std::vector<CallFrame> call_stack_;
    std::unordered_map<std::string, std::shared_ptr<DictObject>> module_cache_;
};

}  // namespace nova
