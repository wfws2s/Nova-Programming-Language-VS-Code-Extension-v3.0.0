#pragma once

#include "lexer/token.hpp"
#include "source/source_location.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nova::ast {

// Forward declarations
class ASTVisitor;

class Node {
public:
    virtual ~Node() = default;
    virtual void accept(ASTVisitor& visitor) = 0;

    SourceSpan span{};
};

// ============================================================================
// Expressions
// ============================================================================

class Expr : public Node {};

enum class LiteralType {
    Number,
    String,
    Boolean,
    Null
};

class LiteralExpr : public Expr {
public:
    LiteralType literal_type;
    double number_value = 0.0;
    std::string string_value;
    bool bool_value = false;

    static std::unique_ptr<LiteralExpr> make_number(double val, SourceSpan span) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal_type = LiteralType::Number;
        node->number_value = val;
        node->span = span;
        return node;
    }

    static std::unique_ptr<LiteralExpr> make_string(std::string val, SourceSpan span) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal_type = LiteralType::String;
        node->string_value = std::move(val);
        node->span = span;
        return node;
    }

    static std::unique_ptr<LiteralExpr> make_bool(bool val, SourceSpan span) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal_type = LiteralType::Boolean;
        node->bool_value = val;
        node->span = span;
        return node;
    }

    static std::unique_ptr<LiteralExpr> make_null(SourceSpan span) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal_type = LiteralType::Null;
        node->span = span;
        return node;
    }

    void accept(ASTVisitor& visitor) override;
};

class VariableExpr : public Expr {
public:
    std::string name;

    VariableExpr(std::string name, SourceSpan span) : name(std::move(name)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class AssignExpr : public Expr {
public:
    std::string name;
    std::unique_ptr<Expr> value;

    AssignExpr(std::string name, std::unique_ptr<Expr> value, SourceSpan span)
        : name(std::move(name)), value(std::move(value)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right, SourceSpan span)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class UnaryExpr : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right, SourceSpan span)
        : op(std::move(op)), right(std::move(right)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class GroupingExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;

    GroupingExpr(std::unique_ptr<Expr> expression, SourceSpan span)
        : expression(std::move(expression)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments, SourceSpan span)
        : callee(std::move(callee)), arguments(std::move(arguments)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class ListExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;

    ListExpr(std::vector<std::unique_ptr<Expr>> elements, SourceSpan span)
        : elements(std::move(elements)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// Compatibility alias for code written before lists were named explicitly.
using ArrayExpr = ListExpr;

class RangeExpr : public Expr {
public:
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;

    RangeExpr(std::unique_ptr<Expr> start, std::unique_ptr<Expr> end, SourceSpan span)
        : start(std::move(start)), end(std::move(end)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class DictExpr : public Expr {
public:
    struct Entry {
        std::unique_ptr<Expr> key;
        std::unique_ptr<Expr> value;
    };
    std::vector<Entry> entries;

    DictExpr(std::vector<Entry> entries, SourceSpan span)
        : entries(std::move(entries)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, SourceSpan span)
        : target(std::move(target)), index(std::move(index)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class IndexAssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;

    IndexAssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value, SourceSpan span)
        : target(std::move(target)), index(std::move(index)), value(std::move(value)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class SliceExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> start; // optional
    std::unique_ptr<Expr> end;   // optional

    SliceExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> start, std::unique_ptr<Expr> end, SourceSpan span)
        : target(std::move(target)), start(std::move(start)), end(std::move(end)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class MemberExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::string member;

    MemberExpr(std::unique_ptr<Expr> target, std::string member, SourceSpan span)
        : target(std::move(target)), member(std::move(member)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// obj.field = value
class MemberAssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::string member;
    std::unique_ptr<Expr> value;

    MemberAssignExpr(std::unique_ptr<Expr> target, std::string member, std::unique_ptr<Expr> value, SourceSpan span)
        : target(std::move(target)), member(std::move(member)), value(std::move(value)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class InterpolatedStringExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> parts;

    InterpolatedStringExpr(std::vector<std::unique_ptr<Expr>> parts, SourceSpan span)
        : parts(std::move(parts)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// new ClassName(args...)
class NewExpr : public Expr {
public:
    std::string class_name;
    std::vector<std::unique_ptr<Expr>> arguments;

    NewExpr(std::string class_name, std::vector<std::unique_ptr<Expr>> arguments, SourceSpan span)
        : class_name(std::move(class_name)), arguments(std::move(arguments)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// type expr — returns type name string
class TypeExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;

    TypeExpr(std::unique_ptr<Expr> expression, SourceSpan span)
        : expression(std::move(expression)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// super.method(args)
class SuperCallExpr : public Expr {
public:
    std::string method;
    std::vector<std::unique_ptr<Expr>> arguments;

    SuperCallExpr(std::string method, std::vector<std::unique_ptr<Expr>> arguments, SourceSpan span)
        : method(std::move(method)), arguments(std::move(arguments)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Statements
// ============================================================================

class Stmt : public Node {};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expression, SourceSpan span)
        : expression(std::move(expression)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class LetStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> initializer;

    LetStmt(std::string name, std::unique_ptr<Expr> initializer, SourceSpan span)
        : name(std::move(name)), initializer(std::move(initializer)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    BlockStmt() = default;
    explicit BlockStmt(SourceSpan span) {
        this->span = span;
    }
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceSpan span)
        : statements(std::move(statements)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

struct ElifBranch {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
    SourceSpan span{};
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> then_branch;
    std::vector<ElifBranch> elif_branches;
    std::unique_ptr<BlockStmt> else_branch; // optional

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<BlockStmt> then_branch,
           std::unique_ptr<BlockStmt> else_branch,
           SourceSpan span)
        : condition(std::move(condition)),
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {
        this->span = span;
    }

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<BlockStmt> then_branch,
           std::vector<ElifBranch> elif_branches,
           std::unique_ptr<BlockStmt> else_branch,
           SourceSpan span)
        : condition(std::move(condition)),
          then_branch(std::move(then_branch)),
          elif_branches(std::move(elif_branches)),
          else_branch(std::move(else_branch)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body, SourceSpan span)
        : condition(std::move(condition)), body(std::move(body)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class ForStmt : public Stmt {
public:
    std::string variable_name;
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<BlockStmt> body;

    ForStmt(std::string variable_name,
            std::unique_ptr<Expr> iterable,
            std::unique_ptr<BlockStmt> body,
            SourceSpan span)
        : variable_name(std::move(variable_name)),
          iterable(std::move(iterable)),
          body(std::move(body)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class FnDeclStmt : public Stmt {
public:
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;

    FnDeclStmt(std::string name,
               std::vector<std::string> params,
               std::shared_ptr<BlockStmt> body,
               SourceSpan span)
        : name(std::move(name)),
          params(std::move(params)),
          body(std::move(body)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // optional

    ReturnStmt(std::unique_ptr<Expr> value, SourceSpan span)
        : value(std::move(value)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class TryCatchStmt : public Stmt {
public:
    std::unique_ptr<BlockStmt> try_branch;
    std::string error_var;
    std::unique_ptr<BlockStmt> catch_branch;

    TryCatchStmt(std::unique_ptr<BlockStmt> try_branch,
                 std::string error_var,
                 std::unique_ptr<BlockStmt> catch_branch,
                 SourceSpan span)
        : try_branch(std::move(try_branch)),
          error_var(std::move(error_var)),
          catch_branch(std::move(catch_branch)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

class ImportStmt : public Stmt {
public:
    std::string module_name;
    std::string alias;
    bool is_path = false;

    ImportStmt(std::string module_name, std::string alias, bool is_path, SourceSpan span)
        : module_name(std::move(module_name)),
          alias(std::move(alias)),
          is_path(is_path) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// global x — declare x is resolved in global scope
class GlobalStmt : public Stmt {
public:
    std::string name;

    GlobalStmt(std::string name, SourceSpan span)
        : name(std::move(name)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// One method (or static fn) declared inside a class body
struct MethodDecl {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    bool is_static = false;
    SourceSpan span{};
};

// class Name [extends Parent] ... methods ... end
class ClassDeclStmt : public Stmt {
public:
    std::string name;
    std::string parent_name; // empty if no inheritance
    std::vector<MethodDecl> methods;

    ClassDeclStmt(std::string name,
                  std::string parent_name,
                  std::vector<MethodDecl> methods,
                  SourceSpan span)
        : name(std::move(name)),
          parent_name(std::move(parent_name)),
          methods(std::move(methods)) {
        this->span = span;
    }

    void accept(ASTVisitor& visitor) override;
};

// break
class BreakStmt : public Stmt {
public:
    explicit BreakStmt(SourceSpan span) {
        this->span = span;
    }
    void accept(ASTVisitor& visitor) override;
};

// continue
class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(SourceSpan span) {
        this->span = span;
    }
    void accept(ASTVisitor& visitor) override;
};

// enum Color RED GREEN BLUE end
class EnumDeclStmt : public Stmt {
public:
    std::string name;
    std::vector<std::string> members;

    EnumDeclStmt(std::string name, std::vector<std::string> members, SourceSpan span)
        : name(std::move(name)), members(std::move(members)) {
        this->span = span;
    }
    void accept(ASTVisitor& visitor) override;
};

// [expr for var in iterable (if cond)?]
class ListComprehensionExpr : public Expr {
public:
    std::unique_ptr<Expr> element;       // expression to evaluate
    std::string variable;                // loop variable name
    std::unique_ptr<Expr> iterable;      // the collection/range to iterate
    std::unique_ptr<Expr> condition;     // optional filter condition (may be null)

    ListComprehensionExpr(std::unique_ptr<Expr> element,
                          std::string variable,
                          std::unique_ptr<Expr> iterable,
                          std::unique_ptr<Expr> condition,
                          SourceSpan span)
        : element(std::move(element)),
          variable(std::move(variable)),
          iterable(std::move(iterable)),
          condition(std::move(condition)) {
        this->span = span;
    }
    void accept(ASTVisitor& visitor) override;
};

class Program : public Node {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    Program() = default;
    explicit Program(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Visitor Interface
// ============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(LiteralExpr& expr) = 0;
    virtual void visit(VariableExpr& expr) = 0;
    virtual void visit(AssignExpr& expr) = 0;
    virtual void visit(BinaryExpr& expr) = 0;
    virtual void visit(UnaryExpr& expr) = 0;
    virtual void visit(GroupingExpr& expr) = 0;
    virtual void visit(CallExpr& expr) = 0;
    virtual void visit(ListExpr& expr) = 0;
    virtual void visit(DictExpr& expr) = 0;
    virtual void visit(RangeExpr& expr) = 0;
    virtual void visit(IndexExpr& expr) = 0;
    virtual void visit(IndexAssignExpr& expr) = 0;
    virtual void visit(SliceExpr& expr) = 0;
    virtual void visit(MemberExpr& expr) = 0;
    virtual void visit(MemberAssignExpr& expr) = 0;
    virtual void visit(InterpolatedStringExpr& expr) = 0;
    virtual void visit(NewExpr& expr) = 0;
    virtual void visit(TypeExpr& expr) = 0;
    virtual void visit(SuperCallExpr& expr) = 0;
    virtual void visit(ListComprehensionExpr& expr) = 0;

    // Statements
    virtual void visit(ExprStmt& stmt) = 0;
    virtual void visit(LetStmt& stmt) = 0;
    virtual void visit(BlockStmt& stmt) = 0;
    virtual void visit(IfStmt& stmt) = 0;
    virtual void visit(WhileStmt& stmt) = 0;
    virtual void visit(ForStmt& stmt) = 0;
    virtual void visit(FnDeclStmt& stmt) = 0;
    virtual void visit(ReturnStmt& stmt) = 0;
    virtual void visit(TryCatchStmt& stmt) = 0;
    virtual void visit(ImportStmt& stmt) = 0;
    virtual void visit(GlobalStmt& stmt) = 0;
    virtual void visit(ClassDeclStmt& stmt) = 0;
    virtual void visit(BreakStmt& stmt) = 0;
    virtual void visit(ContinueStmt& stmt) = 0;
    virtual void visit(EnumDeclStmt& stmt) = 0;
    virtual void visit(Program& program) = 0;
};

}  // namespace nova::ast
