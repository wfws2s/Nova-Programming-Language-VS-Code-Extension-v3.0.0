#include "../src/interpreter/interpreter.hpp"
#include "../src/lexer/lexer.hpp"
#include "../src/parser/parser.hpp"

#include "test_helpers.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace nova;
using namespace nova::test;

static std::string run_code(const std::string& source, bool& had_error, std::string& error_msg,
                            const std::string& input = "") {
    had_error = false;
    error_msg = "";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (lexer.had_error()) {
        had_error = true;
        error_msg = lexer.error().message;
        return "";
    }

    Parser parser(std::move(tokens));
    auto prog = parser.parse_program();
    if (parser.had_error()) {
        had_error = true;
        error_msg = parser.errors()[0].message;
        return "";
    }

    std::ostringstream out;
    std::istringstream in(input);
    Interpreter interp(out, in);
    try {
        interp.interpret(*prog);
    } catch (const NovaRuntimeError& err) {
        had_error = true;
        error_msg = err.message();
        return "";
    }

    return out.str();
}

void test_interp_basic_print() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code("print(\"Hello\", \"NOVA\")", err, err_msg);
    expect_true(!err, "basic print no error");
    expect_eq(out, "Hello NOVA\n", "basic print output");
}

void test_interp_arithmetic() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let a = 10 + 20 * 2
let b = (10 + 20) * 2
let c = 10 / 4
let d = 10 % 3
print(a, b, c, d)
)", err, err_msg);
    expect_true(!err, "arithmetic no error");
    expect_eq(out, "50 60 2.5 1\n", "arithmetic output");
}

void test_interp_string_concat() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let first = "Hello "
let second = "World"
print(first + second)
)", err, err_msg);
    expect_true(!err, "string concat no error");
    expect_eq(out, "Hello World\n", "string concat output");
}

void test_interp_logic_and_comparison() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
print(10 > 5 and 3 < 4)
print(10 < 5 or 3 == 3)
print(not true)
print(not false)
print(not null)
)", err, err_msg);
    expect_true(!err, "logic no error");
    expect_eq(out, "true\ntrue\nfalse\ntrue\ntrue\n", "logic output");
}

void test_interp_if_else() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let age = 20
if age >= 18
    print("Adult")
else
    print("Child")
end

let child_age = 10
if child_age >= 18
    print("Adult")
else
    print("Child")
end
)", err, err_msg);
    expect_true(!err, "if else no error");
    expect_eq(out, "Adult\nChild\n", "if else output");
}

void test_interp_while_loop() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
x = 0
sum = 0
while x <= 5
    sum = sum + x
    x = x + 1
end
print(sum)
)", err, err_msg);
    expect_true(!err, "while no error");
    expect_eq(out, "15\n", "while output");
}

void test_interp_for_loop_and_range() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
sum = 0
for i in 0..5
    sum = sum + i
end
print(sum)
)", err, err_msg);
    expect_true(!err, "for range no error");
    expect_eq(out, "10\n", "for range output (0+1+2+3+4 = 10)");
}

void test_interp_functions_and_recursion() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
fn factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end

print(factorial(5))
print(factorial(6))
)", err, err_msg);
    expect_true(!err, "factorial no error");
    expect_eq(out, "120\n720\n", "factorial output");
}

void test_interp_closures() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
fn make_adder(x)
    fn adder(y)
        return x + y
    end
    return adder
end

let add10 = make_adder(10)
print(add10(5))
print(add10(20))
)", err, err_msg);
    expect_true(!err, "closures no error");
    expect_eq(out, "15\n30\n", "closures output");
}

void test_interp_arrays_and_builtins() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let arr = [10, 20, 30]
print(arr[0], arr[1], arr[2])
print(len(arr))
print(len("hello"))
print(number("42") + 8)
print(string(100) + " items")
)", err, err_msg);
    expect_true(!err, "arrays and builtins no error");
    expect_eq(out, "10 20 30\n3\n5\n50\n100 items\n", "arrays and builtins output");
}

void test_interp_power_lists_dictionaries_and_input() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
numbers = [10, 20, 30]
numbers[-1] = 40
person = {"name": "JIMY",}
person["age"] = 15
print(2 ** 3 ** 2)
print(numbers[-1], "hello"[-1])
print(person["name"], person["age"])
print(input("Age: ") + 1)
)", err, err_msg, "19\n");
    expect_true(!err, "extended collections and input no error");
    expect_eq(out, "512\n40 o\nJIMY 15\nAge: 20\n", "extended collections and input output");
}

void test_error_type_mismatch_add() {
    bool err = false;
    std::string err_msg;
    run_code("let x = 10 + \"hello\"", err, err_msg);
    expect_true(err, "type mismatch add triggers error");
    expect_eq(err_msg, "Cannot add Number and String", "type mismatch add message");
}

void test_error_division_by_zero() {
    bool err = false;
    std::string err_msg;
    run_code("let x = 10 / 0", err, err_msg);
    expect_true(err, "division by zero triggers error");
    expect_eq(err_msg, "Division by zero", "division by zero message");
}

void test_error_undefined_variable() {
    bool err = false;
    std::string err_msg;
    run_code("print(unknown_var)", err, err_msg);
    expect_true(err, "undefined variable triggers error");
    expect_eq(err_msg, "Undefined variable 'unknown_var'", "undefined variable message");
}

void test_assignment_declares_mutable_variable() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code("y = 50\ny = y + 1\nprint(y)", err, err_msg);
    expect_true(!err, "assignment declares and reassigns mutable variable");
    expect_eq(out, "51\n", "assignment declares mutable variable output");
}

void test_error_reassign_constant() {
    bool err = false;
    std::string err_msg;
    run_code("let age = 15\nage = 16", err, err_msg);
    expect_true(err, "constant reassignment triggers error");
    expect_eq(err_msg, "cannot reassign constant 'age'", "constant reassignment message");
}

void test_error_array_index_out_of_bounds() {
    bool err = false;
    std::string err_msg;
    run_code("let a = [1, 2]\nlet x = a[5]", err, err_msg);
    expect_true(err, "index out of bounds triggers error");
    expect_eq(err_msg, "Array index out of bounds: 5 (length: 2)", "index out of bounds message");
}

void test_error_missing_dictionary_key() {
    bool err = false;
    std::string err_msg;
    run_code("person = {}\nprint(person[\"country\"])", err, err_msg);
    expect_true(err, "missing dictionary key triggers error");
    expect_eq(err_msg, "Key not found in dictionary: \"country\"", "missing dictionary key message");
}

void test_stability_guards() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
list = [null]
list[0] = list
dictionary = {}
dictionary["self"] = dictionary
print(list)
print(dictionary)
)", err, err_msg);
    expect_true(!err, "cyclic collections print safely");
    expect_eq(out, "[<cycle>]\n{\"self\": <cycle>}\n", "cyclic collection output");

    run_code("print(0..1000001)", err, err_msg);
    expect_true(err, "oversized range is rejected");
    expect_eq(err_msg, "Range exceeds the maximum collection size of 1000000", "oversized range message");

    run_code("list = [1]\nprint(list[2 ** 1024])", err, err_msg);
    expect_true(err, "non-finite index is rejected");
    expect_eq(err_msg, "Index must be a finite integer, got Infinity", "non-finite index message");

    run_code("list = [1]\nprint(list[2 ** 63])", err, err_msg);
    expect_true(err, "out-of-range finite index is rejected");
    expect_eq(err_msg, "Index must be a finite integer, got 9.2233720368548e+18", "out-of-range finite index message");
}

void test_interp_typeof() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
print(typeof(42))
print(typeof("hi"))
print(typeof(true))
print(typeof(null))
print(typeof([1, 2]))
print(typeof({"a": 1}))
print(typeof(print))
)", err, err_msg);
    expect_true(!err, "typeof no error");
    expect_eq(out, "Number\nString\nBoolean\nNull\nList\nDictionary\nFunction\n", "typeof output");
}

void test_interp_list_conversion() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
print(list("NOVA"))
print(list(null))
print(list(42))
print(list([1, 2]))
)", err, err_msg);
    expect_true(!err, "list conversion no error");
    expect_eq(out, "[\"N\", \"O\", \"V\", \"A\"]\n[]\n[42]\n[1, 2]\n", "list conversion output");
}

void test_interp_split() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
parts = split("red,green,blue", ",")
print(parts)
print(split("a,,b", ","))
)", err, err_msg);
    expect_true(!err, "split no error");
    expect_eq(out, "[\"red\", \"green\", \"blue\"]\n[\"a\", \"\", \"b\"]\n", "split output");
}

void test_interp_append_and_pop() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
items = [10, 20]
append(items, 30)
print(items)
last = pop(items)
print(last, items)
)", err, err_msg);
    expect_true(!err, "append and pop no error");
    expect_eq(out, "[10, 20, 30]\n30 [10, 20]\n", "append and pop output");

    run_code("empty = []\npop(empty)", err, err_msg);
    expect_true(err, "pop empty list error");
    expect_eq(err_msg, "Cannot pop from an empty List", "pop empty list message");
}

void test_interp_keys_and_values() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
person = {"name": "JIMY"}
print(keys(person))
print(values(person))
)", err, err_msg);
    expect_true(!err, "keys and values no error");
    expect_eq(out, "[\"name\"]\n[\"JIMY\"]\n", "keys and values output");
}

void test_interp_dynamic_read() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
age = read("Age: ")
enabled = read("Enabled: ")
scores = read("Scores: ")
person = read("Person: ")
raw_text = read("Text: ")
print(typeof(age), age + 1)
print(typeof(enabled), enabled)
print(typeof(scores), len(scores))
print(typeof(person), person["name"])
print(typeof(raw_text), raw_text)
)", err, err_msg, "15\ntrue\n[10, 20, 30]\n{\"name\": \"JIMY\"}\nhello world\n");
    expect_true(!err, "read dynamic no error");
    expect_eq(out, "Age: Enabled: Scores: Person: Text: Number 16\nBoolean true\nList 3\nDictionary JIMY\nString hello world\n", "read dynamic output");
}

void test_interp_elif() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
fn check_age(age)
    if age >= 18
        print("Adult")
    elif age >= 13
        print("Teen")
    else
        print("Child")
    end
end

check_age(20)
check_age(15)
check_age(8)
)", err, err_msg);
    expect_true(!err, "elif no error");
    expect_eq(out, "Adult\nTeen\nChild\n", "elif output");
}

void test_interp_is_operator() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let age = 15
print(age is "Number")
print(age is "String")
print("hello" is "String")
print([1, 2] is "List")
print({"a": 1} is "Dictionary")
print(null is "Null")
)", err, err_msg);
    expect_true(!err, "is operator no error");
    expect_eq(out, "true\nfalse\ntrue\ntrue\ntrue\ntrue\n", "is operator output");
}

void test_interp_slicing() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
letters = ["a", "b", "c", "d"]
print(letters[1..3])
print(letters[..2])
print(letters[-2..])
print(letters[..])
print("hello"[1..4])
)", err, err_msg);
    expect_true(!err, "slicing no error");
    expect_eq(out, "[\"b\", \"c\"]\n[\"a\", \"b\"]\n[\"c\", \"d\"]\n[\"a\", \"b\", \"c\", \"d\"]\nell\n", "slicing output");
}

void test_interp_string_interpolation() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
let name = "JIMY"
let age = 15
message = "Hello {name}; next year: {age + 1}"
escaped = "Literal: {{name}}"
print(message)
print(escaped)
)", err, err_msg);
    expect_true(!err, "interpolation no error");
    expect_eq(out, "Hello JIMY; next year: 16\nLiteral: {name}\n", "interpolation output");
}

void test_interp_try_catch_and_stack_trace() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
fn divide(a, b)
    return a / b
end

try
    divide(10, 0)
catch error
    print("Caught:", error["type"], error["message"])
    print(typeof(error["stack"]))
end
)", err, err_msg);
    expect_true(!err, "try catch no error");
    expect_eq(out, "Caught: RuntimeError Division by zero\nString\n", "try catch output");
}

void test_interp_modules() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
import math
import string
import list
import dict

print(math.sqrt(9))
print(math.floor(3.7))
print(math.min(10, 5, 20))
print(string.to_upper("nova"))
print(string.join(["a", "b", "c"], "-"))
print(list.reverse([1, 2, 3]))
print(dict.has_key({"x": 10}, "x"))
)", err, err_msg);
    expect_true(!err, "modules no error");
    expect_eq(out, "3\n3\n5\nNOVA\na-b-c\n[3, 2, 1]\ntrue\n", "modules output");
}

void test_interp_builtin_shadowing() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"(
number = number("100")
print(number + 20)
)", err, err_msg);
    expect_true(!err, "builtin shadowing no error");
    expect_eq(out, "120\n", "builtin shadowing output");
}

void test_interp_oop_basic() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
class Person
    fn init(self, name, age)
        self.name = name
        self.age = age
    end

    fn greet(self)
        return "Hi, I am " + self.name + " (" + string(self.age) + ")"
    end

    fn have_birthday(self)
        self.age = self.age + 1
    end
end

let p = Person("Alice", 30)
print(p.name)
print(p.greet())
p.have_birthday()
print(p.greet())
p.hobby = "Coding"
print(p.hobby)
)NOVA", err, err_msg);
    expect_true(!err, "oop basic no error");
    expect_eq(out, "Alice\nHi, I am Alice (30)\nHi, I am Alice (31)\nCoding\n", "oop basic output");
}

void test_interp_oop_inheritance_and_super() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
class Animal
    fn init(self, name)
        self.name = name
    end

    fn speak(self)
        return self.name + " makes a sound"
    end
end

class Dog extends Animal
    fn init(self, name, breed)
        super.init(name)
        self.breed = breed
    end

    fn speak(self)
        return self.name + " (" + self.breed + ") barks!"
    end

    fn original_sound(self)
        return super.speak()
    end
end

let d = Dog("Buddy", "Golden")
print(d.name)
print(d.breed)
print(d.speak())
print(d.original_sound())
)NOVA", err, err_msg);
    expect_true(!err, "oop inheritance no error");
    expect_eq(out, "Buddy\nGolden\nBuddy (Golden) barks!\nBuddy makes a sound\n", "oop inheritance output");
}

void test_interp_oop_static_methods() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
class MathUtil
    static fn add(a, b)
        return a + b
    end

    static fn multiply(a, b)
        return a * b
    end
end

print(MathUtil.add(10, 20))
print(MathUtil.multiply(5, 6))

let inst = MathUtil()
print(inst.add(1, 2))
)NOVA", err, err_msg);
    expect_true(!err, "oop static methods no error");
    expect_eq(out, "30\n30\n3\n", "oop static methods output");
}

void test_interp_type_operator_and_function() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
class TestClass
    fn init(self)
    end
end

let num = 42
let str = "Nova"
let lst = [1, 2]
let dct = {"k": "v"}
let inst = TestClass()

print(type(num))
print(type(str))
print(type(lst))
print(type(dct))
print(type(inst))
print(type(TestClass))
print(type num)
print(type str)
)NOVA", err, err_msg);
    expect_true(!err, "type operator no error");
    expect_eq(out, "Number\nString\nList\nDictionary\nTestClass\nClass\nNumber\nString\n", "type operator output");
}

void test_interp_global_and_local_scope() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
x = 10
let y = 100

fn modify_vars()
    global x
    x = 99
    let y = 200
end

modify_vars()
print(x)
print(y)
)NOVA", err, err_msg);
    expect_true(!err, "global and local scope no error");
    expect_eq(out, "99\n100\n", "global and local scope output");
}

void test_interp_random_module() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
import random as rnd

let r = rnd.random()
print(r >= 0.0 and r < 1.0)

let int_val = rnd.randint(10, 20)
print(int_val >= 10 and int_val <= 20)

let items = ["apple", "banana", "cherry"]
let picked = rnd.choice(items)
print(picked == "apple" or picked == "banana" or picked == "cherry")

let u = rnd.uniform(5.0, 10.0)
print(u >= 5.0 and u <= 10.0)

let shuffled = rnd.shuffle([1, 2, 3, 4])
print(len(shuffled))
)NOVA", err, err_msg);
    expect_true(!err, "random module no error");
    expect_eq(out, "true\ntrue\ntrue\ntrue\n4\n", "random module output");
}

void test_interp_import_external_file() {
    // Create a temporary external module file
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::filesystem::path ext_file = temp_dir / "nova_test_external_helper.nova";
    {
        std::ofstream ofs(ext_file);
        ofs << "class Helper\n";
        ofs << "    fn init(self, tag)\n";
        ofs << "        self.tag = tag\n";
        ofs << "    end\n";
        ofs << "    fn format(self, msg)\n";
        ofs << "        return \"[\" + self.tag + \"] \" + msg\n";
        ofs << "    end\n";
        ofs << "end\n";
        ofs << "fn add_two(x, y)\n";
        ofs << "    return x + y\n";
        ofs << "end\n";
    }

    std::string path_str = ext_file.generic_string();
    std::string code = "import \"" + path_str + "\" as ext\n"
                       "let h = ext.Helper(\"TEST\")\n"
                       "print(h.format(\"hello\"))\n"
                       "print(ext.add_two(10, 25))\n";

    bool err = false;
    std::string err_msg;
    std::string out = run_code(code, err, err_msg);
    expect_true(!err, "external file import no error");
    expect_eq(out, "[TEST] hello\n35\n", "external file import output");

    std::filesystem::remove(ext_file);
}

void test_interp_break_and_continue() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
# test while with break and continue
i = 0
while i < 10
    i = i + 1
    if i == 3
        continue
    end
    if i == 6
        break
    end
    print(i)
end

# test for loop with break and continue
for x in 1..6
    if x == 2
        continue
    end
    if x == 5
        break
    end
    print(x)
end
)NOVA", err, err_msg);
    expect_true(!err, "break and continue no error");
    expect_eq(out, "1\n2\n4\n5\n1\n3\n4\n", "break and continue output");
}

void test_interp_enum() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
enum Color
    RED
    GREEN
    BLUE
end

print(Color.RED)
print(Color.GREEN)
print(Color.BLUE)
print(Color.RED == "RED")
)NOVA", err, err_msg);
    expect_true(!err, "enum declaration no error");
    expect_eq(out, "RED\nGREEN\nBLUE\ntrue\n", "enum declaration output");
}

void test_interp_list_comprehension() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
let squares = [x * x for x in 1..6]
print(squares)

let evens = [x for x in 1..11 if x % 2 == 0]
print(evens)
)NOVA", err, err_msg);
    expect_true(!err, "list comprehension no error");
    expect_eq(out, "[1, 4, 9, 16, 25]\n[2, 4, 6, 8, 10]\n", "list comprehension output");
}

void test_interp_fstring_and_triple_quote() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
let lang = "Nova"
let version = 3
print(f"Welcome to {lang} v0.{version}!")

let multi = """line 1
line 2
line 3"""
print(multi)
)NOVA", err, err_msg);
    expect_true(!err, "fstring and triple quote no error");
    expect_eq(out, "Welcome to Nova v0.3!\nline 1\nline 2\nline 3\n", "fstring and triple quote output");
}

void test_interp_file_and_os_modules() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
import file as f
import os

let test_path = "./temp_test_file.txt"
f.write(test_path, "Hello Nova File System\nSecond line")
print(f.exists(test_path))

let content = f.read(test_path)
print(content)

let lines = f.lines(test_path)
print(len(lines))
print(lines[0])

f.remove(test_path)
print(f.exists(test_path))

print(os.platform is "String")
print(type(os.cwd()))
)NOVA", err, err_msg);
    expect_true(!err, "file and os modules no error");
    expect_eq(out, "true\nHello Nova File System\nSecond line\n2\nHello Nova File System\nfalse\ntrue\nString\n", "file and os modules output");
}

void test_interp_json_and_time_modules() {
    bool err = false;
    std::string err_msg;
    std::string out = run_code(R"NOVA(
import json
import time

let data = {"name": "Nova", "version": 0.3, "active": true, "items": [10, 20]}
let serialized = json.stringify(data)

let parsed = json.parse(serialized)
print(parsed["name"])
print(parsed["active"])
print(parsed["items"][0])

print(time.now() > 0)
)NOVA", err, err_msg);
    expect_true(!err, "json and time modules no error");
    expect_eq(out, "Nova\ntrue\n10\ntrue\n", "json and time modules output");
}

int main() {
    test_interp_basic_print();
    test_interp_arithmetic();
    test_interp_string_concat();
    test_interp_logic_and_comparison();
    test_interp_if_else();
    test_interp_while_loop();
    test_interp_for_loop_and_range();
    test_interp_functions_and_recursion();
    test_interp_closures();
    test_interp_arrays_and_builtins();
    test_interp_power_lists_dictionaries_and_input();
    test_error_type_mismatch_add();
    test_error_division_by_zero();
    test_error_undefined_variable();
    test_assignment_declares_mutable_variable();
    test_error_reassign_constant();
    test_error_array_index_out_of_bounds();
    test_error_missing_dictionary_key();
    test_stability_guards();
    test_interp_typeof();
    test_interp_list_conversion();
    test_interp_split();
    test_interp_append_and_pop();
    test_interp_keys_and_values();
    test_interp_dynamic_read();
    test_interp_elif();
    test_interp_is_operator();
    test_interp_slicing();
    test_interp_string_interpolation();
    test_interp_try_catch_and_stack_trace();
    test_interp_modules();
    test_interp_builtin_shadowing();
    test_interp_oop_basic();
    test_interp_oop_inheritance_and_super();
    test_interp_oop_static_methods();
    test_interp_type_operator_and_function();
    test_interp_global_and_local_scope();
    test_interp_random_module();
    test_interp_import_external_file();
    test_interp_break_and_continue();
    test_interp_enum();
    test_interp_list_comprehension();
    test_interp_fstring_and_triple_quote();
    test_interp_file_and_os_modules();
    test_interp_json_and_time_modules();

    return finish("test_interpreter");
}
