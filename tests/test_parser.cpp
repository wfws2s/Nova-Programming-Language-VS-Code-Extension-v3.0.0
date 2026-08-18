#include "../src/lexer/lexer.hpp"
#include "../src/parser/parser.hpp"

#include "test_helpers.hpp"

using namespace nova;
using namespace nova::test;

static std::unique_ptr<ast::Program> parse_source(const std::string& source, Parser& parser_out) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    parser_out = Parser(std::move(tokens));
    return parser_out.parse_program();
}

void test_parse_literals() {
    Parser parser({});
    auto prog = parse_source("10\n\"hello\"\ntrue\nfalse\nnull", parser);
    expect_true(!parser.had_error(), "literals no error");
    expect_true(prog != nullptr, "literals prog valid");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(5), "5 literal statements");

    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    expect_true(s0 != nullptr, "s0 is ExprStmt");
    auto* l0 = dynamic_cast<ast::LiteralExpr*>(s0->expression.get());
    expect_true(l0 != nullptr, "l0 is LiteralExpr");
    expect_eq(l0->number_value, 10.0, "l0 number value");

    auto* s1 = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    auto* l1 = dynamic_cast<ast::LiteralExpr*>(s1->expression.get());
    expect_eq(l1->string_value, "hello", "l1 string value");
}

void test_parse_let_and_assign() {
    Parser parser({});
    auto prog = parse_source("let x = 42\nx = 100", parser);
    expect_true(!parser.had_error(), "let and assign no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 statements");

    auto* let_stmt = dynamic_cast<ast::LetStmt*>(prog->statements[0].get());
    expect_true(let_stmt != nullptr, "first is LetStmt");
    expect_eq(let_stmt->name, "x", "let var name");

    auto* expr_stmt = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    expect_true(expr_stmt != nullptr, "second is ExprStmt");
    auto* assign_expr = dynamic_cast<ast::AssignExpr*>(expr_stmt->expression.get());
    expect_true(assign_expr != nullptr, "is AssignExpr");
    expect_eq(assign_expr->name, "x", "assign var name");
}

void test_parse_binary_precedence() {
    Parser parser({});
    auto prog = parse_source("1 + 2 * 3", parser);
    expect_true(!parser.had_error(), "precedence no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(1), "1 statement");

    auto* s = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* bin_add = dynamic_cast<ast::BinaryExpr*>(s->expression.get());
    expect_true(bin_add != nullptr, "top is BinaryExpr (+)");
    expect_eq(bin_add->op.type, TokenType::Plus, "op is plus");

    auto* bin_mul = dynamic_cast<ast::BinaryExpr*>(bin_add->right.get());
    expect_true(bin_mul != nullptr, "right is BinaryExpr (*)");
    expect_eq(bin_mul->op.type, TokenType::Star, "op is star");
}

void test_parse_grouping() {
    Parser parser({});
    auto prog = parse_source("(1 + 2) * 3", parser);
    expect_true(!parser.had_error(), "grouping no error");

    auto* s = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* bin_mul = dynamic_cast<ast::BinaryExpr*>(s->expression.get());
    expect_true(bin_mul != nullptr, "top is mul");
    expect_eq(bin_mul->op.type, TokenType::Star, "op is star");

    auto* group = dynamic_cast<ast::GroupingExpr*>(bin_mul->left.get());
    expect_true(group != nullptr, "left is grouping");
}

void test_parse_if_else() {
    Parser parser({});
    auto prog = parse_source(R"(
if x > 10
    print("big")
else
    print("small")
end
)", parser);
    expect_true(!parser.had_error(), "if else no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(1), "1 if stmt");

    auto* if_stmt = dynamic_cast<ast::IfStmt*>(prog->statements[0].get());
    expect_true(if_stmt != nullptr, "is IfStmt");
    expect_true(if_stmt->condition != nullptr, "has condition");
    expect_true(if_stmt->then_branch != nullptr, "has then branch");
    expect_true(if_stmt->else_branch != nullptr, "has else branch");
}

void test_parse_while_loop() {
    Parser parser({});
    auto prog = parse_source(R"(
while x < 10
    x = x + 1
end
)", parser);
    expect_true(!parser.had_error(), "while no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(1), "1 while stmt");

    auto* while_stmt = dynamic_cast<ast::WhileStmt*>(prog->statements[0].get());
    expect_true(while_stmt != nullptr, "is WhileStmt");
    expect_eq(while_stmt->body->statements.size(), static_cast<std::size_t>(1), "1 stmt in body");
}

void test_parse_for_in_range() {
    Parser parser({});
    auto prog = parse_source(R"(
for i in 0..10
    print(i)
end
)", parser);
    expect_true(!parser.had_error(), "for range no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(1), "1 for stmt");

    auto* for_stmt = dynamic_cast<ast::ForStmt*>(prog->statements[0].get());
    expect_true(for_stmt != nullptr, "is ForStmt");
    expect_eq(for_stmt->variable_name, "i", "var is i");
    auto* range_expr = dynamic_cast<ast::RangeExpr*>(for_stmt->iterable.get());
    expect_true(range_expr != nullptr, "iterable is RangeExpr");
}

void test_parse_function_declaration_and_call() {
    Parser parser({});
    auto prog = parse_source(R"(
fn add(a, b)
    return a + b
end

let res = add(5, 10)
)", parser);
    expect_true(!parser.had_error(), "fn decl and call no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 stmts");

    auto* fn_stmt = dynamic_cast<ast::FnDeclStmt*>(prog->statements[0].get());
    expect_true(fn_stmt != nullptr, "is FnDeclStmt");
    expect_eq(fn_stmt->name, "add", "fn name is add");
    expect_eq(fn_stmt->params.size(), static_cast<std::size_t>(2), "2 params");
    expect_eq(fn_stmt->params[0], "a", "param 0 is a");
    expect_eq(fn_stmt->params[1], "b", "param 1 is b");
}

void test_parse_arrays_and_index() {
    Parser parser({});
    auto prog = parse_source(R"(
let arr = [10, 20, 30]
let first = arr[0]
)", parser);
    expect_true(!parser.had_error(), "arrays no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 stmts");

    auto* s0 = dynamic_cast<ast::LetStmt*>(prog->statements[0].get());
    auto* arr_expr = dynamic_cast<ast::ArrayExpr*>(s0->initializer.get());
    expect_true(arr_expr != nullptr, "is ArrayExpr");
    expect_eq(arr_expr->elements.size(), static_cast<std::size_t>(3), "3 elements");

    auto* s1 = dynamic_cast<ast::LetStmt*>(prog->statements[1].get());
    auto* idx_expr = dynamic_cast<ast::IndexExpr*>(s1->initializer.get());
    expect_true(idx_expr != nullptr, "is IndexExpr");
}

void test_parse_power_precedence() {
    Parser parser({});
    auto prog = parse_source("2 ** 3 ** 2", parser);
    expect_true(!parser.had_error(), "power precedence no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(1), "1 statement");

    auto* s = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* top_pow = dynamic_cast<ast::BinaryExpr*>(s->expression.get());
    expect_true(top_pow != nullptr, "top is BinaryExpr (**)");
    expect_eq(top_pow->op.type, TokenType::StarStar, "op is StarStar");

    // Right-associativity: right child should be 3 ** 2
    auto* right_pow = dynamic_cast<ast::BinaryExpr*>(top_pow->right.get());
    expect_true(right_pow != nullptr, "right child is StarStar (right-associative)");
    expect_eq(right_pow->op.type, TokenType::StarStar, "right op is StarStar");
}

void test_parse_dictionary_and_trailing_commas() {
    Parser parser({});
    auto prog = parse_source(R"(
person = {
    "name": "JIMY",
    "age": 15,
}
numbers = [10, 20,]
)", parser);
    expect_true(!parser.had_error(), "dict and trailing commas no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 stmts");

    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* assign_dict = dynamic_cast<ast::AssignExpr*>(s0->expression.get());
    expect_true(assign_dict != nullptr, "s0 is AssignExpr");
    auto* dict_expr = dynamic_cast<ast::DictExpr*>(assign_dict->value.get());
    expect_true(dict_expr != nullptr, "val is DictExpr");
    expect_eq(dict_expr->entries.size(), static_cast<std::size_t>(2), "2 dict entries");

    auto* s1 = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    auto* assign_arr = dynamic_cast<ast::AssignExpr*>(s1->expression.get());
    auto* arr_expr = dynamic_cast<ast::ArrayExpr*>(assign_arr->value.get());
    expect_true(arr_expr != nullptr, "val is ArrayExpr");
    expect_eq(arr_expr->elements.size(), static_cast<std::size_t>(2), "2 arr elements");
}

void test_parse_index_assignment() {
    Parser parser({});
    auto prog = parse_source(R"(
numbers[0] = 100
person["age"] = 16
)", parser);
    expect_true(!parser.had_error(), "index assign no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 stmts");

    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* idx_assign0 = dynamic_cast<ast::IndexAssignExpr*>(s0->expression.get());
    expect_true(idx_assign0 != nullptr, "s0 is IndexAssignExpr");

    auto* s1 = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    auto* idx_assign1 = dynamic_cast<ast::IndexAssignExpr*>(s1->expression.get());
    expect_true(idx_assign1 != nullptr, "s1 is IndexAssignExpr");
}

void test_parse_error_missing_end() {
    Parser parser({});
    parse_source("if x > 5\nprint(x)\n", parser);
    expect_true(parser.had_error(), "missing end reports error");
}

void test_empty_tokens_and_invalid_number_are_safe() {
    Parser empty_parser({});
    auto empty_program = empty_parser.parse_program();
    expect_true(!empty_parser.had_error(), "empty token list parses as an empty program");
    expect_eq(empty_program->statements.size(), static_cast<std::size_t>(0), "empty token program has no statements");

    Parser parser({});
    parse_source(std::string(400, '9'), parser);
    expect_true(parser.had_error(), "out-of-range number reports a parser error");
}

void test_parse_elif() {
    Parser parser({});
    auto prog = parse_source(R"(
if x > 10
    print("big")
elif x > 5
    print("medium")
elif x > 0
    print("small")
else
    print("zero or negative")
end
)", parser);
    expect_true(!parser.had_error(), "elif no error");
    auto* if_stmt = dynamic_cast<ast::IfStmt*>(prog->statements[0].get());
    expect_true(if_stmt != nullptr, "is IfStmt");
    expect_eq(if_stmt->elif_branches.size(), static_cast<std::size_t>(2), "2 elif branches");
}

void test_parse_slicing() {
    Parser parser({});
    auto prog = parse_source(R"(
a = list[1..3]
b = list[..2]
c = list[-2..]
d = list[..]
)", parser);
    expect_true(!parser.had_error(), "slicing no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(4), "4 slice statements");
    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* a0 = dynamic_cast<ast::AssignExpr*>(s0->expression.get());
    auto* sl0 = dynamic_cast<ast::SliceExpr*>(a0->value.get());
    expect_true(sl0 != nullptr, "is SliceExpr");
    expect_true(sl0->start != nullptr, "sl0 has start");
    expect_true(sl0->end != nullptr, "sl0 has end");

    auto* s1 = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    auto* a1 = dynamic_cast<ast::AssignExpr*>(s1->expression.get());
    auto* sl1 = dynamic_cast<ast::SliceExpr*>(a1->value.get());
    expect_true(sl1->start == nullptr, "sl1 has no start");
    expect_true(sl1->end != nullptr, "sl1 has end");
}

void test_parse_member_dot() {
    Parser parser({});
    auto prog = parse_source("math.sqrt(9)", parser);
    expect_true(!parser.had_error(), "dot access no error");
    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* call = dynamic_cast<ast::CallExpr*>(s0->expression.get());
    expect_true(call != nullptr, "is CallExpr");
    auto* member = dynamic_cast<ast::MemberExpr*>(call->callee.get());
    expect_true(member != nullptr, "callee is MemberExpr");
    expect_eq(member->member, "sqrt", "member name is sqrt");
}

void test_parse_is_operator() {
    Parser parser({});
    auto prog = parse_source("age is \"Number\"", parser);
    expect_true(!parser.had_error(), "is operator no error");
    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* bin = dynamic_cast<ast::BinaryExpr*>(s0->expression.get());
    expect_true(bin != nullptr, "is BinaryExpr");
    expect_eq(bin->op.type, TokenType::Is, "op is Is");
}

void test_parse_try_catch() {
    Parser parser({});
    auto prog = parse_source(R"(
try
    score = number(input("Score: "))
catch error
    print(error["type"], error["message"])
end
)", parser);
    expect_true(!parser.had_error(), "try catch no error");
    auto* tc = dynamic_cast<ast::TryCatchStmt*>(prog->statements[0].get());
    expect_true(tc != nullptr, "is TryCatchStmt");
    expect_eq(tc->error_var, "error", "error var is 'error'");
    expect_true(tc->try_branch != nullptr, "has try branch");
    expect_true(tc->catch_branch != nullptr, "has catch branch");
}

void test_parse_import() {
    Parser parser({});
    auto prog = parse_source(R"(
import math
import "./helpers.nova" as helpers
)", parser);
    expect_true(!parser.had_error(), "import no error");
    expect_eq(prog->statements.size(), static_cast<std::size_t>(2), "2 import statements");

    auto* imp0 = dynamic_cast<ast::ImportStmt*>(prog->statements[0].get());
    expect_true(imp0 != nullptr, "is ImportStmt");
    expect_eq(imp0->module_name, "math", "imp0 module");
    expect_eq(imp0->alias, "math", "imp0 alias");
    expect_true(!imp0->is_path, "imp0 not path");

    auto* imp1 = dynamic_cast<ast::ImportStmt*>(prog->statements[1].get());
    expect_true(imp1 != nullptr, "imp1 is ImportStmt");
    expect_eq(imp1->module_name, "./helpers.nova", "imp1 module");
    expect_eq(imp1->alias, "helpers", "imp1 alias");
    expect_true(imp1->is_path, "imp1 is path");
}

void test_parse_string_interpolation() {
    Parser parser({});
    auto prog = parse_source(R"(
message = "Hello {name}; next year: {age + 1}"
escaped = "Braces: {{literal}}"
)", parser);
    expect_true(!parser.had_error(), "interpolation no error");
    auto* s0 = dynamic_cast<ast::ExprStmt*>(prog->statements[0].get());
    auto* a0 = dynamic_cast<ast::AssignExpr*>(s0->expression.get());
    auto* interp = dynamic_cast<ast::InterpolatedStringExpr*>(a0->value.get());
    expect_true(interp != nullptr, "is InterpolatedStringExpr");

    auto* s1 = dynamic_cast<ast::ExprStmt*>(prog->statements[1].get());
    auto* a1 = dynamic_cast<ast::AssignExpr*>(s1->expression.get());
    auto* lit = dynamic_cast<ast::LiteralExpr*>(a1->value.get());
    expect_true(lit != nullptr, "escaped is LiteralExpr");
    expect_eq(lit->string_value, "Braces: {literal}", "escaped braces parsed correctly");
}

void test_parse_reserved_name_error() {
    Parser parser({});
    parse_source("let if = 10", parser);
    expect_true(parser.had_error(), "let keyword reports error");
    expect_eq(parser.errors()[0].category, "ReservedNameError", "let keyword category");

    Parser p2({});
    parse_source("fn else()\nend", p2);
    expect_true(p2.had_error(), "fn keyword reports error");
    expect_eq(p2.errors()[0].category, "ReservedNameError", "fn keyword category");

    Parser p3({});
    parse_source("for while in 1..5\nend", p3);
    expect_true(p3.had_error(), "for keyword reports error");
    expect_eq(p3.errors()[0].category, "ReservedNameError", "for keyword category");

    Parser p4({});
    parse_source("try\nprint(1)\ncatch fn\nprint(2)\nend", p4);
    expect_true(p4.had_error(), "catch keyword reports error");
    expect_eq(p4.errors()[0].category, "ReservedNameError", "catch keyword category");
}

void test_parse_class_and_inheritance() {
    Parser parser({});
    auto prog = parse_source(R"(
class Dog extends Animal
    fn init(self, name)
        self.name = name
    end

    static fn species()
        return "Canis"
    end
end
)", parser);

    expect_true(!parser.had_error(), "class parse no error");
    expect_eq(prog->statements.size(), std::size_t(1), "program has 1 class stmt");
    auto* cls = dynamic_cast<ast::ClassDeclStmt*>(prog->statements[0].get());
    expect_true(cls != nullptr, "statement is ClassDeclStmt");
    if (cls) {
        expect_eq(cls->name, "Dog", "class name is Dog");
        expect_eq(cls->parent_name, "Animal", "parent name is Animal");
        expect_eq(cls->methods.size(), std::size_t(2), "class has 2 methods");
        expect_eq(cls->methods[0].name, "init", "method 1 is init");
        expect_true(!cls->methods[0].is_static, "method 1 is not static");
        expect_eq(cls->methods[1].name, "species", "method 2 is species");
        expect_true(cls->methods[1].is_static, "method 2 is static");
    }
}

void test_parse_global_and_type_and_super() {
    Parser parser({});
    auto prog = parse_source(R"(
global counter
let t = type 123
let s = super.init("foo")
)", parser);

    expect_true(!parser.had_error(), "global and type parse no error");
    expect_eq(prog->statements.size(), std::size_t(3), "program has 3 stmts");
    auto* g = dynamic_cast<ast::GlobalStmt*>(prog->statements[0].get());
    expect_true(g != nullptr, "statement 1 is GlobalStmt");
    if (g) {
        expect_eq(g->name, "counter", "global variable name");
    }
}

int main() {
    test_parse_literals();
    test_parse_let_and_assign();
    test_parse_binary_precedence();
    test_parse_power_precedence();
    test_parse_grouping();
    test_parse_if_else();
    test_parse_while_loop();
    test_parse_for_in_range();
    test_parse_function_declaration_and_call();
    test_parse_arrays_and_index();
    test_parse_dictionary_and_trailing_commas();
    test_parse_index_assignment();
    test_parse_error_missing_end();
    test_empty_tokens_and_invalid_number_are_safe();
    test_parse_elif();
    test_parse_slicing();
    test_parse_member_dot();
    test_parse_is_operator();
    test_parse_try_catch();
    test_parse_import();
    test_parse_string_interpolation();
    test_parse_reserved_name_error();
    test_parse_class_and_inheritance();
    test_parse_global_and_type_and_super();

    return finish("test_parser");
}
