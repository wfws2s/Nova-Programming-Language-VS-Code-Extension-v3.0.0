#pragma once

#include "ast/ast.hpp"
#include "vm/chunk.hpp"

#include <memory>
#include <string>

namespace nova::vm {

class Compiler : public ast::ASTVisitor {
public:
    Compiler();

    std::unique_ptr<Chunk> compile(ast::Program& program);

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
    void visit(ast::ListComprehensionExpr& expr) override;

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
    void visit(ast::BreakStmt& stmt) override;
    void visit(ast::ContinueStmt& stmt) override;
    void visit(ast::EnumDeclStmt& stmt) override;
    void visit(ast::Program& program) override;

private:
    void emit_byte(uint8_t byte, int line = 1);
    void emit_op(OpCode op, int line = 1);
    void emit_constant(Value value, int line = 1);
    int emit_jump(OpCode instruction, int line = 1);
    void patch_jump(int offset);
    void emit_loop(int loop_start, int line = 1);

    std::unique_ptr<Chunk> chunk_;
};

}  // namespace nova::vm
