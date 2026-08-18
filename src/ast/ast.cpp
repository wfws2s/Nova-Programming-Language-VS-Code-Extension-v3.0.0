#include "ast/ast.hpp"

namespace nova::ast {

void LiteralExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void VariableExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AssignExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void GroupingExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ListExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DictExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void RangeExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IndexExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IndexAssignExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SliceExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MemberExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MemberAssignExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void InterpolatedStringExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NewExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TypeExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SuperCallExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ExprStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void LetStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BlockStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FnDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TryCatchStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ImportStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void GlobalStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ClassDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

}  // namespace nova::ast
