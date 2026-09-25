#include "vm/compiler.hpp"

namespace nova::vm {

Compiler::Compiler() : chunk_(std::make_unique<Chunk>()) {}

std::unique_ptr<Chunk> Compiler::compile(ast::Program& program) {
    chunk_ = std::make_unique<Chunk>();
    program.accept(*this);
    emit_op(OpCode::OP_NIL);
    emit_op(OpCode::OP_RETURN);
    return std::move(chunk_);
}

void Compiler::emit_byte(uint8_t byte, int line) {
    chunk_->write(byte, line);
}

void Compiler::emit_op(OpCode op, int line) {
    chunk_->write_op(op, line);
}

void Compiler::emit_constant(Value value, int line) {
    int constant = chunk_->add_constant(std::move(value));
    emit_op(OpCode::OP_CONSTANT, line);
    emit_byte(static_cast<uint8_t>(constant), line);
}

int Compiler::emit_jump(OpCode instruction, int line) {
    emit_op(instruction, line);
    emit_byte(0xff, line);
    emit_byte(0xff, line);
    return static_cast<int>(chunk_->code().size() - 2);
}

void Compiler::patch_jump(int offset) {
    int jump = static_cast<int>(chunk_->code().size()) - offset - 2;
    chunk_->code()[offset] = static_cast<uint8_t>((jump >> 8) & 0xff);
    chunk_->code()[offset + 1] = static_cast<uint8_t>(jump & 0xff);
}

void Compiler::emit_loop(int loop_start, int line) {
    emit_op(OpCode::OP_LOOP, line);
    int jump = static_cast<int>(chunk_->code().size()) - loop_start + 2;
    emit_byte(static_cast<uint8_t>((jump >> 8) & 0xff), line);
    emit_byte(static_cast<uint8_t>(jump & 0xff), line);
}

void Compiler::visit(ast::LiteralExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    switch (expr.literal_type) {
        case ast::LiteralType::Number:
            emit_constant(Value(expr.number_value), line);
            break;
        case ast::LiteralType::String:
            emit_constant(Value(expr.string_value), line);
            break;
        case ast::LiteralType::Boolean:
            emit_op(expr.bool_value ? OpCode::OP_TRUE : OpCode::OP_FALSE, line);
            break;
        case ast::LiteralType::Null:
            emit_op(OpCode::OP_NIL, line);
            break;
    }
}

void Compiler::visit(ast::VariableExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    int name_const = chunk_->add_constant(Value(expr.name));
    emit_op(OpCode::OP_GET_GLOBAL, line);
    emit_byte(static_cast<uint8_t>(name_const), line);
}

void Compiler::visit(ast::AssignExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    if (expr.value) expr.value->accept(*this);
    int name_const = chunk_->add_constant(Value(expr.name));
    emit_op(OpCode::OP_SET_GLOBAL, line);
    emit_byte(static_cast<uint8_t>(name_const), line);
}

void Compiler::visit(ast::BinaryExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);

    switch (expr.op.type) {
        case TokenType::Plus: emit_op(OpCode::OP_ADD, line); break;
        case TokenType::Minus: emit_op(OpCode::OP_SUBTRACT, line); break;
        case TokenType::Star: emit_op(OpCode::OP_MULTIPLY, line); break;
        case TokenType::Slash: emit_op(OpCode::OP_DIVIDE, line); break;
        case TokenType::Percent: emit_op(OpCode::OP_MODULO, line); break;
        case TokenType::StarStar: emit_op(OpCode::OP_POWER, line); break;
        case TokenType::EqualEqual: emit_op(OpCode::OP_EQUAL, line); break;
        case TokenType::BangEqual:
            emit_op(OpCode::OP_EQUAL, line);
            emit_op(OpCode::OP_NOT, line);
            break;
        case TokenType::Greater: emit_op(OpCode::OP_GREATER, line); break;
        case TokenType::Less: emit_op(OpCode::OP_LESS, line); break;
        default: break;
    }
}

void Compiler::visit(ast::UnaryExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    if (expr.right) expr.right->accept(*this);
    if (expr.op.type == TokenType::Minus) {
        emit_op(OpCode::OP_NEGATE, line);
    } else if (expr.op.type == TokenType::Not) {
        emit_op(OpCode::OP_NOT, line);
    }
}

void Compiler::visit(ast::GroupingExpr& expr) {
    if (expr.expression) expr.expression->accept(*this);
}

void Compiler::visit(ast::CallExpr& expr) {
    int line = expr.span.start.line > 0 ? expr.span.start.line : 1;
    if (expr.callee) expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
    emit_op(OpCode::OP_CALL, line);
    emit_byte(static_cast<uint8_t>(expr.arguments.size()), line);
}

void Compiler::visit(ast::ListExpr& /*expr*/) {}
void Compiler::visit(ast::DictExpr& /*expr*/) {}
void Compiler::visit(ast::RangeExpr& /*expr*/) {}
void Compiler::visit(ast::IndexExpr& /*expr*/) {}
void Compiler::visit(ast::IndexAssignExpr& /*expr*/) {}
void Compiler::visit(ast::SliceExpr& /*expr*/) {}
void Compiler::visit(ast::MemberExpr& /*expr*/) {}
void Compiler::visit(ast::MemberAssignExpr& /*expr*/) {}
void Compiler::visit(ast::InterpolatedStringExpr& /*expr*/) {}
void Compiler::visit(ast::NewExpr& /*expr*/) {}
void Compiler::visit(ast::TypeExpr& /*expr*/) {}
void Compiler::visit(ast::SuperCallExpr& /*expr*/) {}
void Compiler::visit(ast::ListComprehensionExpr& /*expr*/) {}

void Compiler::visit(ast::ExprStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
        emit_op(OpCode::OP_POP, stmt.span.start.line > 0 ? stmt.span.start.line : 1);
    }
}

void Compiler::visit(ast::LetStmt& stmt) {
    int line = stmt.span.start.line > 0 ? stmt.span.start.line : 1;
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        emit_op(OpCode::OP_NIL, line);
    }
    int name_const = chunk_->add_constant(Value(stmt.name));
    emit_op(OpCode::OP_DEFINE_GLOBAL, line);
    emit_byte(static_cast<uint8_t>(name_const), line);
}

void Compiler::visit(ast::BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        if (s) s->accept(*this);
    }
}

void Compiler::visit(ast::IfStmt& stmt) {
    int line = stmt.span.start.line > 0 ? stmt.span.start.line : 1;
    if (stmt.condition) stmt.condition->accept(*this);

    int then_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE, line);
    emit_op(OpCode::OP_POP, line);

    if (stmt.then_branch) stmt.then_branch->accept(*this);

    int else_jump = emit_jump(OpCode::OP_JUMP, line);
    patch_jump(then_jump);
    emit_op(OpCode::OP_POP, line);

    if (stmt.else_branch) stmt.else_branch->accept(*this);
    patch_jump(else_jump);
}

void Compiler::visit(ast::WhileStmt& stmt) {
    int line = stmt.span.start.line > 0 ? stmt.span.start.line : 1;
    int loop_start = static_cast<int>(chunk_->code().size());

    if (stmt.condition) stmt.condition->accept(*this);
    int exit_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE, line);
    emit_op(OpCode::OP_POP, line);

    if (stmt.body) stmt.body->accept(*this);
    emit_loop(loop_start, line);

    patch_jump(exit_jump);
    emit_op(OpCode::OP_POP, line);
}

void Compiler::visit(ast::ForStmt& /*stmt*/) {}
void Compiler::visit(ast::FnDeclStmt& /*stmt*/) {}

void Compiler::visit(ast::ReturnStmt& stmt) {
    int line = stmt.span.start.line > 0 ? stmt.span.start.line : 1;
    if (stmt.value) {
        stmt.value->accept(*this);
    } else {
        emit_op(OpCode::OP_NIL, line);
    }
    emit_op(OpCode::OP_RETURN, line);
}

void Compiler::visit(ast::TryCatchStmt& /*stmt*/) {}
void Compiler::visit(ast::ImportStmt& /*stmt*/) {}
void Compiler::visit(ast::GlobalStmt& /*stmt*/) {}
void Compiler::visit(ast::ClassDeclStmt& /*stmt*/) {}
void Compiler::visit(ast::BreakStmt& /*stmt*/) {}
void Compiler::visit(ast::ContinueStmt& /*stmt*/) {}
void Compiler::visit(ast::EnumDeclStmt& /*stmt*/) {}

void Compiler::visit(ast::Program& program) {
    for (const auto& stmt : program.statements) {
        if (stmt) stmt->accept(*this);
    }
}

}  // namespace nova::vm
