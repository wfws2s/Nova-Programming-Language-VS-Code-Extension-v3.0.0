#include "interpreter/interpreter.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/builtins.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <thread>

namespace nova {

namespace {

constexpr std::size_t kMaxCollectionElements = 1'000'000;
constexpr std::size_t kMaxLoopIterations = 10'000'000;

long long require_integer(const Value& value, const SourceSpan& span, const std::string& description) {
    if (!value.is_number()) {
        throw NovaRuntimeError(RuntimeErrorType::TypeError,
                               description + " must be a Number, got " + value.type_name(), span);
    }
    const double number = value.as_number();
    if (!std::isfinite(number) || std::trunc(number) != number ||
        number < static_cast<double>(std::numeric_limits<long long>::min()) ||
        number >= 9'223'372'036'854'775'808.0) {
        throw NovaRuntimeError(RuntimeErrorType::IndexError,
                               description + " must be a finite integer, got " + value.to_string(), span);
    }
    return static_cast<long long>(number);
}

void require_within_collection_limit(unsigned long long size, const SourceSpan& span) {
    if (size > kMaxCollectionElements) {
        throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                               "Range exceeds the maximum collection size of " +
                                   std::to_string(kMaxCollectionElements), span);
    }
}

std::string json_stringify_helper(const Value& val) {
    if (val.is_null()) return "null";
    if (val.is_bool()) return val.as_bool() ? "true" : "false";
    if (val.is_number()) {
        std::ostringstream ss;
        double d = val.as_number();
        if (std::trunc(d) == d && std::abs(d) < 1e15) {
            ss << static_cast<long long>(d);
        } else {
            ss << d;
        }
        return ss.str();
    }
    if (val.is_string()) {
        std::string s = "\"";
        for (char c : val.as_string()) {
            if (c == '"') s += "\\\"";
            else if (c == '\\') s += "\\\\";
            else if (c == '\n') s += "\\n";
            else if (c == '\r') s += "\\r";
            else if (c == '\t') s += "\\t";
            else s += c;
        }
        s += "\"";
        return s;
    }
    if (val.is_list()) {
        auto list = val.as_list();
        std::string s = "[";
        for (std::size_t i = 0; i < list->size(); ++i) {
            if (i > 0) s += ", ";
            s += json_stringify_helper((*list)[i]);
        }
        s += "]";
        return s;
    }
    if (val.is_dict()) {
        auto dict = val.as_dict();
        std::string s = "{";
        bool first = true;
        for (const auto& [k, v] : *dict) {
            if (!first) s += ", ";
            first = false;
            s += "\"";
            for (char c : k) {
                if (c == '"') s += "\\\"";
                else if (c == '\\') s += "\\\\";
                else if (c == '\n') s += "\\n";
                else if (c == '\r') s += "\\r";
                else if (c == '\t') s += "\\t";
                else s += c;
            }
            s += "\": ";
            s += json_stringify_helper(v);
        }
        s += "}";
        return s;
    }
    return "\"" + val.to_string() + "\"";
}

class SimpleJsonParser {
public:
    explicit SimpleJsonParser(std::string_view src) : src_(src) {}

    Value parse(const SourceSpan& span) {
        skip_whitespace();
        if (pos_ >= src_.size()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Empty JSON input", span);
        }
        Value val = parse_value(span);
        skip_whitespace();
        if (pos_ < src_.size()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unexpected trailing characters in JSON", span);
        }
        return val;
    }

private:
    void skip_whitespace() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t' || src_[pos_] == '\n' || src_[pos_] == '\r')) {
            ++pos_;
        }
    }

    Value parse_value(const SourceSpan& span) {
        skip_whitespace();
        if (pos_ >= src_.size()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unexpected end of JSON", span);
        }
        char c = src_[pos_];
        if (c == 'n') {
            if (match("null")) return Value();
        } else if (c == 't') {
            if (match("true")) return Value(true);
        } else if (c == 'f') {
            if (match("false")) return Value(false);
        } else if (c == '"') {
            return Value(parse_string(span));
        } else if (c == '[') {
            return Value(parse_array(span));
        } else if (c == '{') {
            return Value(parse_object(span));
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return Value(parse_number(span));
        }
        throw NovaRuntimeError(RuntimeErrorType::RuntimeError, std::string("json.parse(): Unexpected character '") + c + "' in JSON", span);
    }

    bool match(std::string_view text) {
        if (src_.substr(pos_, text.size()) == text) {
            pos_ += text.size();
            return true;
        }
        return false;
    }

    std::string parse_string(const SourceSpan& span) {
        if (src_[pos_] != '"') {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Expected '\"'", span);
        }
        ++pos_; // skip '"'
        std::string res;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            char c = src_[pos_++];
            if (c == '\\') {
                if (pos_ >= src_.size()) {
                    throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unterminated string escape", span);
                }
                char esc = src_[pos_++];
                switch (esc) {
                    case '"': res.push_back('"'); break;
                    case '\\': res.push_back('\\'); break;
                    case '/': res.push_back('/'); break;
                    case 'b': res.push_back('\b'); break;
                    case 'f': res.push_back('\f'); break;
                    case 'n': res.push_back('\n'); break;
                    case 'r': res.push_back('\r'); break;
                    case 't': res.push_back('\t'); break;
                    default: res.push_back(esc); break;
                }
            } else {
                res.push_back(c);
            }
        }
        if (pos_ >= src_.size() || src_[pos_] != '"') {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unterminated string in JSON", span);
        }
        ++pos_; // consume closing '"'
        return res;
    }

    double parse_number(const SourceSpan& span) {
        std::size_t start = pos_;
        if (src_[pos_] == '-') ++pos_;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        }
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        }
        std::string num_str(src_.substr(start, pos_ - start));
        try {
            return std::stod(num_str);
        } catch (...) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Invalid number '" + num_str + "'", span);
        }
    }

    std::shared_ptr<ListObject> parse_array(const SourceSpan& span) {
        ++pos_; // skip '['
        auto list = std::make_shared<ListObject>();
        skip_whitespace();
        if (pos_ < src_.size() && src_[pos_] == ']') {
            ++pos_;
            return list;
        }
        while (pos_ < src_.size()) {
            list->push_back(parse_value(span));
            skip_whitespace();
            if (pos_ < src_.size() && src_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
            } else if (pos_ < src_.size() && src_[pos_] == ']') {
                ++pos_;
                return list;
            } else {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Expected ',' or ']' in array", span);
            }
        }
        throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unterminated array in JSON", span);
    }

    std::shared_ptr<DictObject> parse_object(const SourceSpan& span) {
        ++pos_; // skip '{'
        auto dict = std::make_shared<DictObject>();
        skip_whitespace();
        if (pos_ < src_.size() && src_[pos_] == '}') {
            ++pos_;
            return dict;
        }
        while (pos_ < src_.size()) {
            skip_whitespace();
            if (pos_ >= src_.size() || src_[pos_] != '"') {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Expected string key in JSON object", span);
            }
            std::string key = parse_string(span);
            skip_whitespace();
            if (pos_ >= src_.size() || src_[pos_] != ':') {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Expected ':' after key in JSON object", span);
            }
            ++pos_; // skip ':'
            Value val = parse_value(span);
            (*dict)[std::move(key)] = std::move(val);
            skip_whitespace();
            if (pos_ < src_.size() && src_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
            } else if (pos_ < src_.size() && src_[pos_] == '}') {
                ++pos_;
                return dict;
            } else {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Expected ',' or '}' in object", span);
            }
        }
        throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "json.parse(): Unterminated object in JSON", span);
    }

    std::string_view src_;
    std::size_t pos_ = 0;
};

Value parse_json_string(const std::string& input, const SourceSpan& span) {
    SimpleJsonParser parser(input);
    return parser.parse(span);
}

}  // namespace

Interpreter::Interpreter(std::ostream& output_stream, std::istream& input_stream)
    : globals_(Environment::create()), environment_(globals_), out_(output_stream), in_(input_stream) {
    Builtins::register_all(*globals_, out_, in_);
}

void Interpreter::interpret(ast::Program& program) {
    program.accept(*this);
}

Value Interpreter::evaluate(ast::Expr& expr) {
    expr.accept(*this);
    return last_value_;
}

void Interpreter::execute(ast::Stmt& stmt) {
    stmt.accept(*this);
}

void Interpreter::execute_block(const std::vector<std::unique_ptr<ast::Stmt>>& statements,
                                std::shared_ptr<Environment> environment) {
    std::shared_ptr<Environment> previous = environment_;
    try {
        environment_ = environment;
        for (const auto& stmt : statements) {
            if (stmt) {
                execute(*stmt);
            }
        }
        environment_ = previous;
    } catch (...) {
        environment_ = previous;
        throw;
    }
}

std::string Interpreter::format_stack_trace(const SourceSpan& failing_span) const {
    std::ostringstream ss;
    ss << "Stack trace:\n";
    int line = failing_span.start.line > 0 ? failing_span.start.line : 1;
    int col = failing_span.start.column > 0 ? failing_span.start.column : 1;

    if (call_stack_.empty()) {
        ss << "  at <main> (" << current_filename_ << ":" << line << ":" << col << ")";
        return ss.str();
    }

    const auto& top = call_stack_.back();
    ss << "  at " << top.function_name << " (" << top.filename << ":" << line << ":" << col << ")\n";

    for (std::size_t i = call_stack_.size() - 1; i > 0; --i) {
        const auto& caller = call_stack_[i - 1];
        const auto& callee = call_stack_[i];
        int call_line = callee.call_span.start.line > 0 ? callee.call_span.start.line : 1;
        int call_col = callee.call_span.start.column > 0 ? callee.call_span.start.column : 1;
        ss << "  called from " << caller.function_name << " (" << caller.filename << ":" << call_line << ":" << call_col << ")\n";
    }

    int main_line = call_stack_[0].call_span.start.line > 0 ? call_stack_[0].call_span.start.line : 1;
    int main_col = call_stack_[0].call_span.start.column > 0 ? call_stack_[0].call_span.start.column : 1;
    ss << "  called from <main> (" << current_filename_ << ":" << main_line << ":" << main_col << ")";

    return ss.str();
}

std::shared_ptr<DictObject> Interpreter::get_or_create_std_module(const std::string& name) {
    auto it = module_cache_.find(name);
    if (it != module_cache_.end()) {
        return it->second;
    }

    if (name == "math") {
        auto dict = std::make_shared<DictObject>();
        (*dict)["pi"] = Value(3.14159265358979323846);
        (*dict)["e"] = Value(2.71828182845904523536);

        (*dict)["sqrt"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.sqrt() requires 1 Number argument", span);
            }
            if (args[0].as_number() < 0.0) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "math.sqrt() domain error on negative value", span);
            }
            return Value(std::sqrt(args[0].as_number()));
        }, "math.sqrt");

        (*dict)["pow"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_number() || !args[1].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.pow() requires 2 Number arguments", span);
            }
            return Value(std::pow(args[0].as_number(), args[1].as_number()));
        }, "math.pow");

        (*dict)["floor"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.floor() requires 1 Number argument", span);
            }
            return Value(std::floor(args[0].as_number()));
        }, "math.floor");

        (*dict)["ceil"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.ceil() requires 1 Number argument", span);
            }
            return Value(std::ceil(args[0].as_number()));
        }, "math.ceil");

        (*dict)["round"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.round() requires 1 Number argument", span);
            }
            return Value(std::round(args[0].as_number()));
        }, "math.round");

        (*dict)["abs"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.abs() requires 1 Number argument", span);
            }
            return Value(std::abs(args[0].as_number()));
        }, "math.abs");

        (*dict)["sin"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.sin() requires 1 Number argument", span);
            }
            return Value(std::sin(args[0].as_number()));
        }, "math.sin");

        (*dict)["cos"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.cos() requires 1 Number argument", span);
            }
            return Value(std::cos(args[0].as_number()));
        }, "math.cos");

        (*dict)["tan"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.tan() requires 1 Number argument", span);
            }
            return Value(std::tan(args[0].as_number()));
        }, "math.tan");

        (*dict)["min"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.empty()) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "math.min() requires at least 1 argument", span);
            }
            double min_val = std::numeric_limits<double>::infinity();
            for (const auto& a : args) {
                if (!a.is_number()) throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.min() arguments must be Numbers", span);
                if (a.as_number() < min_val) min_val = a.as_number();
            }
            return Value(min_val);
        }, "math.min");

        (*dict)["max"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.empty()) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "math.max() requires at least 1 argument", span);
            }
            double max_val = -std::numeric_limits<double>::infinity();
            for (const auto& a : args) {
                if (!a.is_number()) throw NovaRuntimeError(RuntimeErrorType::TypeError, "math.max() arguments must be Numbers", span);
                if (a.as_number() > max_val) max_val = a.as_number();
            }
            return Value(max_val);
        }, "math.max");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "string") {
        auto dict = std::make_shared<DictObject>();
        (*dict)["to_upper"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.to_upper() requires 1 String argument", span);
            }
            std::string res = args[0].as_string();
            for (char& c : res) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return Value(std::move(res));
        }, "string.to_upper");

        (*dict)["to_lower"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.to_lower() requires 1 String argument", span);
            }
            std::string res = args[0].as_string();
            for (char& c : res) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return Value(std::move(res));
        }, "string.to_lower");

        (*dict)["trim"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.trim() requires 1 String argument", span);
            }
            const std::string& s = args[0].as_string();
            std::size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
            std::size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
            return Value(s.substr(start, end - start));
        }, "string.trim");

        (*dict)["starts_with"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.starts_with() requires 2 String arguments", span);
            }
            const std::string& str = args[0].as_string();
            const std::string& prefix = args[1].as_string();
            if (prefix.size() > str.size()) return Value(false);
            return Value(str.compare(0, prefix.size(), prefix) == 0);
        }, "string.starts_with");

        (*dict)["ends_with"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.ends_with() requires 2 String arguments", span);
            }
            const std::string& str = args[0].as_string();
            const std::string& suffix = args[1].as_string();
            if (suffix.size() > str.size()) return Value(false);
            return Value(str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
        }, "string.ends_with");

        (*dict)["contains"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.contains() requires 2 String arguments", span);
            }
            return Value(args[0].as_string().find(args[1].as_string()) != std::string::npos);
        }, "string.contains");

        (*dict)["join"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_list() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "string.join() requires a List and a String delimiter", span);
            }
            auto list = args[0].as_list();
            const std::string& delim = args[1].as_string();
            std::string res;
            for (std::size_t i = 0; i < list->size(); ++i) {
                if (i > 0) res += delim;
                res += (*list)[i].to_string();
            }
            return Value(std::move(res));
        }, "string.join");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "list") {
        auto dict = std::make_shared<DictObject>();
        (*dict)["contains"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_list()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "list.contains() requires a List as first argument", span);
            }
            auto list = args[0].as_list();
            for (const auto& elem : *list) {
                if (elem.equals(args[1])) return Value(true);
            }
            return Value(false);
        }, "list.contains");

        (*dict)["index_of"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_list()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "list.index_of() requires a List as first argument", span);
            }
            auto list = args[0].as_list();
            for (std::size_t i = 0; i < list->size(); ++i) {
                if ((*list)[i].equals(args[1])) return Value(static_cast<double>(i));
            }
            return Value(-1.0);
        }, "list.index_of");

        (*dict)["reverse"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_list()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "list.reverse() requires 1 List argument", span);
            }
            auto list = args[0].as_list();
            auto res = std::make_shared<ListObject>(*list);
            std::reverse(res->begin(), res->end());
            return Value(res);
        }, "list.reverse");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "dict") {
        auto dict = std::make_shared<DictObject>();
        (*dict)["has_key"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_dict() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "dict.has_key() requires a Dictionary and a String key", span);
            }
            auto d = args[0].as_dict();
            return Value(d->find(args[1].as_string()) != d->end());
        }, "dict.has_key");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "random") {
        auto dict = std::make_shared<DictObject>();

        (*dict)["random"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            (void)args; (void)span;
            static thread_local std::mt19937_64 rng(std::random_device{}());
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return Value(dist(rng));
        }, "random.random");

        (*dict)["randint"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_number() || !args[1].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.randint() requires 2 Number arguments (min, max)", span);
            }
            static thread_local std::mt19937_64 rng(std::random_device{}());
            long long a = static_cast<long long>(args[0].as_number());
            long long b = static_cast<long long>(args[1].as_number());
            if (a > b) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "random.randint(a, b) empty range because a > b", span);
            }
            std::uniform_int_distribution<long long> dist(a, b);
            return Value(static_cast<double>(dist(rng)));
        }, "random.randint");

        (*dict)["choice"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_list()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.choice() requires a List argument", span);
            }
            auto list = args[0].as_list();
            if (list->empty()) {
                throw NovaRuntimeError(RuntimeErrorType::IndexError, "Cannot choose from an empty list", span);
            }
            static thread_local std::mt19937_64 rng(std::random_device{}());
            std::uniform_int_distribution<std::size_t> dist(0, list->size() - 1);
            return (*list)[dist(rng)];
        }, "random.choice");

        (*dict)["shuffle"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_list()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.shuffle() requires a List argument", span);
            }
            auto list = args[0].as_list();
            static thread_local std::mt19937_64 rng(std::random_device{}());
            std::shuffle(list->begin(), list->end(), rng);
            return Value(list);
        }, "random.shuffle");

        (*dict)["uniform"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_number() || !args[1].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.uniform() requires 2 Number arguments (a, b)", span);
            }
            static thread_local std::mt19937_64 rng(std::random_device{}());
            double a = args[0].as_number();
            double b = args[1].as_number();
            if (a > b) std::swap(a, b);
            std::uniform_real_distribution<double> dist(a, b);
            return Value(dist(rng));
        }, "random.uniform");

        (*dict)["range"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.empty() || args.size() > 2) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "random.range() requires 1 or 2 Number arguments", span);
            }
            static thread_local std::mt19937_64 rng(std::random_device{}());
            long long start = 0;
            long long stop = 0;
            if (args.size() == 1) {
                if (!args[0].is_number()) throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.range() requires Number", span);
                stop = static_cast<long long>(args[0].as_number());
            } else {
                if (!args[0].is_number() || !args[1].is_number()) throw NovaRuntimeError(RuntimeErrorType::TypeError, "random.range() requires Numbers", span);
                start = static_cast<long long>(args[0].as_number());
                stop = static_cast<long long>(args[1].as_number());
            }
            if (start >= stop) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "random.range() empty range", span);
            }
            std::uniform_int_distribution<long long> dist(start, stop - 1);
            return Value(static_cast<double>(dist(rng)));
        }, "random.range");

        (*dict)["randrange"] = (*dict)["range"];

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "file" || name == "io") {
        auto dict = std::make_shared<DictObject>();

        (*dict)["read"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.read() requires 1 String path argument", span);
            }
            std::filesystem::path p(args[0].as_string());
            if (!std::filesystem::exists(p)) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.read(): File not found: " + p.string(), span);
            }
            std::ifstream file(p, std::ios::binary);
            if (!file) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.read(): Could not open file: " + p.string(), span);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return Value(buffer.str());
        }, "file.read");

        (*dict)["write"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.write() requires 2 String arguments (path, content)", span);
            }
            std::filesystem::path p(args[0].as_string());
            std::ofstream file(p, std::ios::binary);
            if (!file) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.write(): Could not open file for writing: " + p.string(), span);
            }
            file << args[1].as_string();
            return Value();
        }, "file.write");

        (*dict)["append"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.append() requires 2 String arguments (path, content)", span);
            }
            std::filesystem::path p(args[0].as_string());
            std::ofstream file(p, std::ios::app | std::ios::binary);
            if (!file) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.append(): Could not open file for appending: " + p.string(), span);
            }
            file << args[1].as_string();
            return Value();
        }, "file.append");

        (*dict)["exists"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.exists() requires 1 String path argument", span);
            }
            std::filesystem::path p(args[0].as_string());
            return Value(std::filesystem::exists(p));
        }, "file.exists");

        (*dict)["remove"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.remove() requires 1 String path argument", span);
            }
            std::filesystem::path p(args[0].as_string());
            std::error_code ec;
            bool ok = std::filesystem::remove(p, ec);
            return Value(ok);
        }, "file.remove");

        (*dict)["lines"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "file.lines() requires 1 String path argument", span);
            }
            std::filesystem::path p(args[0].as_string());
            if (!std::filesystem::exists(p)) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.lines(): File not found: " + p.string(), span);
            }
            std::ifstream file(p);
            if (!file) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "file.lines(): Could not open file: " + p.string(), span);
            }
            auto list = std::make_shared<ListObject>();
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                list->push_back(Value(line));
            }
            return Value(list);
        }, "file.lines");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "os" || name == "sys") {
        auto dict = std::make_shared<DictObject>();

#if defined(_WIN32)
        (*dict)["platform"] = Value("windows");
#elif defined(__APPLE__)
        (*dict)["platform"] = Value("macos");
#else
        (*dict)["platform"] = Value("linux");
#endif

        (*dict)["args"] = Value(std::make_shared<ListObject>());

        (*dict)["env"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "os.env() requires 1 String variable name", span);
            }
            const char* val = std::getenv(args[0].as_string().c_str());
            return val ? Value(std::string(val)) : Value();
        }, "os.env");

        (*dict)["cwd"] = Value([](const std::vector<Value>&, const SourceSpan&) -> Value {
            return Value(std::filesystem::current_path().string());
        }, "os.cwd");

        (*dict)["exit"] = Value([](const std::vector<Value>& args, const SourceSpan&) -> Value {
            int code = 0;
            if (!args.empty() && args[0].is_number()) {
                code = static_cast<int>(args[0].as_number());
            }
            std::exit(code);
            return Value();
        }, "os.exit");

        (*dict)["exec"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "os.exec() requires 1 String command argument", span);
            }
            std::string cmd = args[0].as_string();
            std::string result;
            char buffer[256];
#if defined(_WIN32)
            FILE* pipe = _popen(cmd.c_str(), "r");
#else
            FILE* pipe = popen(cmd.c_str(), "r");
#endif
            if (!pipe) {
                throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "os.exec(): Failed to execute command", span);
            }
            while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
#if defined(_WIN32)
            _pclose(pipe);
#else
            pclose(pipe);
#endif
            return Value(result);
        }, "os.exec");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "time") {
        auto dict = std::make_shared<DictObject>();

        (*dict)["now"] = Value([](const std::vector<Value>&, const SourceSpan&) -> Value {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            double secs = std::chrono::duration<double>(duration).count();
            return Value(secs);
        }, "time.now");

        (*dict)["sleep"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_number()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "time.sleep() requires 1 Number argument (milliseconds)", span);
            }
            long long ms = static_cast<long long>(args[0].as_number());
            if (ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            return Value();
        }, "time.sleep");

        (*dict)["clock"] = Value([](const std::vector<Value>&, const SourceSpan&) -> Value {
            auto now = std::chrono::steady_clock::now();
            auto duration = now.time_since_epoch();
            double secs = std::chrono::duration<double>(duration).count();
            return Value(secs);
        }, "time.clock");

        module_cache_[name] = dict;
        return dict;
    }

    if (name == "json") {
        auto dict = std::make_shared<DictObject>();

        (*dict)["stringify"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1) {
                throw NovaRuntimeError(RuntimeErrorType::ArgumentError, "json.stringify() requires 1 argument", span);
            }
            return Value(json_stringify_helper(args[0]));
        }, "json.stringify");

        (*dict)["parse"] = Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
            if (args.size() != 1 || !args[0].is_string()) {
                throw NovaRuntimeError(RuntimeErrorType::TypeError, "json.parse() requires 1 String argument", span);
            }
            return parse_json_string(args[0].as_string(), span);
        }, "json.parse");

        module_cache_[name] = dict;
        return dict;
    }

    return nullptr;
}

void Interpreter::visit(ast::LiteralExpr& expr) {
    switch (expr.literal_type) {
        case ast::LiteralType::Number:
            last_value_ = Value(expr.number_value);
            break;
        case ast::LiteralType::String:
            last_value_ = Value(expr.string_value);
            break;
        case ast::LiteralType::Boolean:
            last_value_ = Value(expr.bool_value);
            break;
        case ast::LiteralType::Null:
            last_value_ = Value();
            break;
    }
}

void Interpreter::visit(ast::VariableExpr& expr) {
    last_value_ = environment_->get(expr.name, expr.span);
}

void Interpreter::visit(ast::AssignExpr& expr) {
    Value val = evaluate(*expr.value);
    environment_->assign(expr.name, val, expr.span);
    last_value_ = val;
}

void Interpreter::visit(ast::GroupingExpr& expr) {
    last_value_ = evaluate(*expr.expression);
}

void Interpreter::visit(ast::UnaryExpr& expr) {
    Value right = evaluate(*expr.right);

    if (expr.op.type == TokenType::Minus) {
        if (!right.is_number()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Unary '-' operand must be a Number, got " + std::string(right.type_name()),
                expr.span
            );
        }
        last_value_ = Value(-right.as_number());
        return;
    }

    if (expr.op.type == TokenType::Not) {
        last_value_ = Value(!right.is_truthy());
        return;
    }

    throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "Unknown unary operator", expr.span);
}

void Interpreter::visit(ast::BinaryExpr& expr) {
    // Logical short-circuiting
    if (expr.op.type == TokenType::Or) {
        Value left = evaluate(*expr.left);
        if (left.is_truthy()) {
            last_value_ = left;
            return;
        }
        last_value_ = evaluate(*expr.right);
        return;
    }

    if (expr.op.type == TokenType::And) {
        Value left = evaluate(*expr.left);
        if (!left.is_truthy()) {
            last_value_ = left;
            return;
        }
        last_value_ = evaluate(*expr.right);
        return;
    }

    Value left = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    if (expr.op.type == TokenType::Is) {
        if (!right.is_string()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Right-hand side of 'is' must be a String type name, got " + std::string(right.type_name()),
                expr.span
            );
        }
        const std::string& type_name = right.as_string();
        if (type_name != "Number" && type_name != "String" && type_name != "Boolean" &&
            type_name != "Null" && type_name != "List" && type_name != "Dictionary" &&
            type_name != "Function") {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Invalid type name '" + type_name + "' in 'is' check",
                expr.span
            );
        }
        last_value_ = Value(std::string(left.type_name()) == type_name);
        return;
    }

    switch (expr.op.type) {
        case TokenType::Plus:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() + right.as_number());
                return;
            }
            if (left.is_string() && right.is_string()) {
                last_value_ = Value(left.as_string() + right.as_string());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot add " + std::string(left.type_name()) + " and " + std::string(right.type_name()),
                expr.span,
                "Convert values to matching types before adding."
            );

        case TokenType::Minus:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() - right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot subtract " + std::string(right.type_name()) + " from " + std::string(left.type_name()),
                expr.span
            );

        case TokenType::Star:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() * right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot multiply " + std::string(left.type_name()) + " and " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::Slash:
            if (left.is_number() && right.is_number()) {
                if (right.as_number() == 0.0) {
                    throw NovaRuntimeError(
                        RuntimeErrorType::RuntimeError,
                        "Division by zero",
                        expr.span
                    );
                }
                last_value_ = Value(left.as_number() / right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot divide " + std::string(left.type_name()) + " by " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::Percent:
            if (left.is_number() && right.is_number()) {
                if (right.as_number() == 0.0) {
                    throw NovaRuntimeError(
                        RuntimeErrorType::RuntimeError,
                        "Modulo by zero",
                        expr.span
                    );
                }
                last_value_ = Value(std::fmod(left.as_number(), right.as_number()));
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot calculate modulo of " + std::string(left.type_name()) + " and " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::StarStar:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(std::pow(left.as_number(), right.as_number()));
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot exponentiate " + std::string(left.type_name()) + " and " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::EqualEqual:
            last_value_ = Value(left.equals(right));
            return;

        case TokenType::BangEqual:
            last_value_ = Value(!left.equals(right));
            return;

        case TokenType::Less:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() < right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot compare " + std::string(left.type_name()) + " < " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::LessEqual:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() <= right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot compare " + std::string(left.type_name()) + " <= " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::Greater:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() > right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot compare " + std::string(left.type_name()) + " > " + std::string(right.type_name()),
                expr.span
            );

        case TokenType::GreaterEqual:
            if (left.is_number() && right.is_number()) {
                last_value_ = Value(left.as_number() >= right.as_number());
                return;
            }
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot compare " + std::string(left.type_name()) + " >= " + std::string(right.type_name()),
                expr.span
            );

        default:
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError, "Unknown binary operator", expr.span);
    }
}

void Interpreter::visit(ast::RangeExpr& expr) {
    Value start_val = evaluate(*expr.start);
    Value end_val = evaluate(*expr.end);

    long long start = require_integer(start_val, expr.start->span, "Range start");
    long long end = require_integer(end_val, expr.end->span, "Range end");

    const unsigned long long count = start <= end
        ? static_cast<unsigned long long>(end) - static_cast<unsigned long long>(start)
        : static_cast<unsigned long long>(start) - static_cast<unsigned long long>(end);
    require_within_collection_limit(count, expr.span);

    auto elements = std::make_shared<ArrayObject>();
    if (start <= end) {
        for (long long i = start; i < end; ++i) {
            elements->push_back(Value(static_cast<double>(i)));
        }
    } else {
        for (long long i = start; i > end; --i) {
            elements->push_back(Value(static_cast<double>(i)));
        }
    }

    last_value_ = Value(elements);
}

void Interpreter::visit(ast::ListExpr& expr) {
    auto elements = std::make_shared<ArrayObject>();
    for (const auto& elem_expr : expr.elements) {
        elements->push_back(evaluate(*elem_expr));
    }
    last_value_ = Value(elements);
}

void Interpreter::visit(ast::DictExpr& expr) {
    auto dictionary = std::make_shared<DictObject>();
    for (const auto& entry : expr.entries) {
        Value key = evaluate(*entry.key);
        if (!key.is_string()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Dictionary key must be a String, got " + std::string(key.type_name()),
                entry.key->span
            );
        }
        (*dictionary)[key.as_string()] = evaluate(*entry.value);
    }
    last_value_ = Value(dictionary);
}

void Interpreter::visit(ast::IndexExpr& expr) {
    Value target = evaluate(*expr.target);
    Value index = evaluate(*expr.index);

    if (target.is_dict()) {
        if (!index.is_string()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Dictionary key must be a String, got " + std::string(index.type_name()),
                expr.span
            );
        }
        auto dictionary = target.as_dict();
        auto it = dictionary->find(index.as_string());
        if (it == dictionary->end()) {
            throw NovaRuntimeError(
                RuntimeErrorType::KeyError,
                "Key not found in dictionary: \"" + index.as_string() + "\"",
                expr.span
            );
        }
        last_value_ = it->second;
        return;
    }

    long long idx = require_integer(index, expr.index->span, "Index");

    if (target.is_array()) {
        auto arr = target.as_array();
        if (idx < 0) {
            idx += static_cast<long long>(arr->size());
        }
        if (idx < 0 || static_cast<std::size_t>(idx) >= arr->size()) {
            throw NovaRuntimeError(
                RuntimeErrorType::IndexError,
                "Array index out of bounds: " + std::to_string(idx) + " (length: " + std::to_string(arr->size()) + ")",
                expr.span
            );
        }
        last_value_ = (*arr)[static_cast<std::size_t>(idx)];
        return;
    }

    if (target.is_string()) {
        const std::string& str = target.as_string();
        if (idx < 0) {
            idx += static_cast<long long>(str.size());
        }
        if (idx < 0 || static_cast<std::size_t>(idx) >= str.size()) {
            throw NovaRuntimeError(
                RuntimeErrorType::IndexError,
                "String index out of bounds: " + std::to_string(idx) + " (length: " + std::to_string(str.size()) + ")",
                expr.span
            );
        }
        last_value_ = Value(std::string(1, str[static_cast<std::size_t>(idx)]));
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::TypeError,
        "Cannot index into " + std::string(target.type_name()),
        expr.span
    );
}

void Interpreter::visit(ast::IndexAssignExpr& expr) {
    Value target = evaluate(*expr.target);
    Value index = evaluate(*expr.index);
    Value value = evaluate(*expr.value);

    if (target.is_dict()) {
        if (!index.is_string()) {
            throw NovaRuntimeError(RuntimeErrorType::TypeError,
                                   "Dictionary key must be a String, got " + std::string(index.type_name()),
                                   expr.index->span);
        }
        (*target.as_dict())[index.as_string()] = value;
        last_value_ = value;
        return;
    }

    if (!target.is_list()) {
        throw NovaRuntimeError(RuntimeErrorType::TypeError,
                               "Cannot assign through index on " + std::string(target.type_name()), expr.span);
    }
    auto list = target.as_list();
    long long position = require_integer(index, expr.index->span, "List index");
    if (position < 0) {
        position += static_cast<long long>(list->size());
    }
    if (position < 0 || static_cast<std::size_t>(position) >= list->size()) {
        throw NovaRuntimeError(RuntimeErrorType::IndexError,
                               "List index out of bounds: " + std::to_string(position) +
                                   " (length: " + std::to_string(list->size()) + ")", expr.span);
    }
    (*list)[static_cast<std::size_t>(position)] = value;
    last_value_ = value;
}

void Interpreter::visit(ast::SliceExpr& expr) {
    Value target = evaluate(*expr.target);

    if (target.is_list()) {
        auto list = target.as_list();
        long long len = static_cast<long long>(list->size());

        long long start = 0;
        if (expr.start) {
            start = require_integer(evaluate(*expr.start), expr.start->span, "Slice start");
            if (start < 0) start += len;
            start = std::clamp(start, 0LL, len);
        }

        long long end = len;
        if (expr.end) {
            end = require_integer(evaluate(*expr.end), expr.end->span, "Slice end");
            if (end < 0) end += len;
            end = std::clamp(end, 0LL, len);
        }

        auto result = std::make_shared<ListObject>();
        if (start < end) {
            for (long long i = start; i < end; ++i) {
                result->push_back((*list)[static_cast<std::size_t>(i)]);
            }
        }
        last_value_ = Value(result);
        return;
    }

    if (target.is_string()) {
        const std::string& str = target.as_string();
        long long len = static_cast<long long>(str.size());

        long long start = 0;
        if (expr.start) {
            start = require_integer(evaluate(*expr.start), expr.start->span, "Slice start");
            if (start < 0) start += len;
            start = std::clamp(start, 0LL, len);
        }

        long long end = len;
        if (expr.end) {
            end = require_integer(evaluate(*expr.end), expr.end->span, "Slice end");
            if (end < 0) end += len;
            end = std::clamp(end, 0LL, len);
        }

        if (start < end) {
            last_value_ = Value(str.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start)));
        } else {
            last_value_ = Value("");
        }
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::TypeError,
        "Cannot slice " + std::string(target.type_name()),
        expr.span
    );
}

void Interpreter::visit(ast::MemberExpr& expr) {
    Value target = evaluate(*expr.target);

    if (target.is_instance()) {
        auto inst = target.as_instance();
        if (inst && inst->fields) {
            auto it = inst->fields->find(expr.member);
            if (it != inst->fields->end()) {
                last_value_ = it->second;
                return;
            }
        }

        // Look for method in class hierarchy
        std::shared_ptr<ClassObject> curr = inst->klass;
        while (curr) {
            auto mit = curr->methods.find(expr.member);
            if (mit != curr->methods.end()) {
                auto m_fn = mit->second;
                BuiltinFn bound = [this, inst, m_fn](const std::vector<Value>& args, const SourceSpan& span) -> Value {
                    bool has_self_param = !m_fn->params.empty() && m_fn->params[0] == "self";
                    std::size_t expected_args = has_self_param ? (m_fn->params.size() - 1) : m_fn->params.size();
                    if (args.size() != expected_args) {
                        throw NovaRuntimeError(
                            RuntimeErrorType::ArgumentError,
                            "Method '" + m_fn->name + "' expected " + std::to_string(expected_args) +
                            " arguments, got " + std::to_string(args.size()),
                            span
                        );
                    }
                    auto fn_env = Environment::create(m_fn->closure);
                    fn_env->define("self", Value(inst));
                    std::size_t offset = has_self_param ? 1 : 0;
                    for (std::size_t i = 0; i < args.size(); ++i) {
                        fn_env->define(m_fn->params[i + offset], args[i]);
                    }
                    call_stack_.push_back(CallFrame{m_fn->name, current_filename_, span});
                    try {
                        execute_block(m_fn->body->statements, fn_env);
                        call_stack_.pop_back();
                        return Value();
                    } catch (const ReturnSignal& sig) {
                        call_stack_.pop_back();
                        return sig.value;
                    } catch (...) {
                        call_stack_.pop_back();
                        throw;
                    }
                };
                last_value_ = Value(bound, (inst->klass ? inst->klass->name : "") + "." + expr.member);
                return;
            }

            auto sit = curr->statics.find(expr.member);
            if (sit != curr->statics.end()) {
                last_value_ = sit->second;
                return;
            }

            curr = curr->parent;
        }

        throw NovaRuntimeError(
            RuntimeErrorType::NameError,
            "Property or method '" + expr.member + "' not found on instance of " +
            (inst->klass ? inst->klass->name : std::string("Class")),
            expr.span
        );
    }

    if (target.is_class()) {
        auto cls = target.as_class();
        std::shared_ptr<ClassObject> curr = cls;
        while (curr) {
            auto it = curr->statics.find(expr.member);
            if (it != curr->statics.end()) {
                last_value_ = it->second;
                return;
            }
            curr = curr->parent;
        }

        throw NovaRuntimeError(
            RuntimeErrorType::NameError,
            "Static member '" + expr.member + "' not found on class '" + cls->name + "'",
            expr.span
        );
    }

    if (target.is_dict()) {
        auto dict = target.as_dict();
        auto it = dict->find(expr.member);
        if (it == dict->end()) {
            throw NovaRuntimeError(
                RuntimeErrorType::KeyError,
                "Key not found in dictionary: \"" + expr.member + "\"",
                expr.span
            );
        }
        last_value_ = it->second;
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::TypeError,
        "Cannot access member '" + expr.member + "' on " + std::string(target.type_name()),
        expr.span
    );
}

void Interpreter::visit(ast::MemberAssignExpr& expr) {
    Value target = evaluate(*expr.target);
    Value val = evaluate(*expr.value);

    if (target.is_instance()) {
        auto inst = target.as_instance();
        if (!inst->fields) {
            inst->fields = std::make_shared<DictObject>();
        }
        (*inst->fields)[expr.member] = val;
        last_value_ = val;
        return;
    }

    if (target.is_class()) {
        auto cls = target.as_class();
        cls->statics[expr.member] = val;
        last_value_ = val;
        return;
    }

    if (target.is_dict()) {
        auto dict = target.as_dict();
        (*dict)[expr.member] = val;
        last_value_ = val;
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::TypeError,
        "Cannot assign member '" + expr.member + "' on " + std::string(target.type_name()),
        expr.span
    );
}

void Interpreter::visit(ast::InterpolatedStringExpr& expr) {
    std::string result;
    for (const auto& part : expr.parts) {
        Value val = evaluate(*part);
        result += val.to_string();
    }
    last_value_ = Value(std::move(result));
}

void Interpreter::visit(ast::NewExpr& expr) {
    Value cls_val = environment_->get(expr.class_name, expr.span);
    if (!cls_val.is_class()) {
        throw NovaRuntimeError(
            RuntimeErrorType::TypeError,
            "'" + expr.class_name + "' is not a class",
            expr.span
        );
    }
    auto cls = cls_val.as_class();
    std::vector<Value> arguments;
    for (const auto& arg_expr : expr.arguments) {
        arguments.push_back(evaluate(*arg_expr));
    }

    auto inst = std::make_shared<InstanceObject>();
    inst->klass = cls;
    inst->fields = std::make_shared<DictObject>();

    std::shared_ptr<FunctionObject> init_fn = nullptr;
    std::shared_ptr<ClassObject> curr = cls;
    while (curr) {
        auto it = curr->methods.find("init");
        if (it != curr->methods.end()) {
            init_fn = it->second;
            break;
        }
        curr = curr->parent;
    }

    if (init_fn) {
        bool has_self_param = !init_fn->params.empty() && init_fn->params[0] == "self";
        std::size_t expected_args = has_self_param ? (init_fn->params.size() - 1) : init_fn->params.size();
        if (arguments.size() != expected_args) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "Class '" + cls->name + "' constructor init() expected " + std::to_string(expected_args) +
                " arguments, got " + std::to_string(arguments.size()),
                expr.span
            );
        }

        auto fn_env = Environment::create(init_fn->closure);
        fn_env->define("self", Value(inst));
        std::size_t offset = has_self_param ? 1 : 0;
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            fn_env->define(init_fn->params[i + offset], arguments[i]);
        }

        call_stack_.push_back(CallFrame{cls->name + ".init", current_filename_, expr.span});
        try {
            execute_block(init_fn->body->statements, fn_env);
            call_stack_.pop_back();
        } catch (const ReturnSignal&) {
            call_stack_.pop_back();
        } catch (...) {
            call_stack_.pop_back();
            throw;
        }
    } else if (!arguments.empty()) {
        throw NovaRuntimeError(
            RuntimeErrorType::ArgumentError,
            "Class '" + cls->name + "' has no constructor but was called with arguments",
            expr.span
        );
    }

    last_value_ = Value(inst);
}

void Interpreter::visit(ast::TypeExpr& expr) {
    Value val = evaluate(*expr.expression);
    if (val.is_instance() && val.as_instance()->klass) {
        last_value_ = Value(val.as_instance()->klass->name);
    } else {
        last_value_ = Value(std::string(val.type_name()));
    }
}

void Interpreter::visit(ast::SuperCallExpr& expr) {
    Value self_val = environment_->get("self", expr.span);
    if (!self_val.is_instance()) {
        throw NovaRuntimeError(
            RuntimeErrorType::RuntimeError,
            "'super' can only be used inside instance methods",
            expr.span
        );
    }
    auto inst = self_val.as_instance();
    if (!inst->klass || !inst->klass->parent) {
        throw NovaRuntimeError(
            RuntimeErrorType::RuntimeError,
            "Class '" + (inst->klass ? inst->klass->name : std::string("?")) + "' does not inherit from any parent class",
            expr.span
        );
    }

    std::string method_name = expr.method.empty() ? "init" : expr.method;
    std::shared_ptr<FunctionObject> parent_fn = nullptr;
    std::shared_ptr<ClassObject> curr = inst->klass->parent;
    while (curr) {
        auto it = curr->methods.find(method_name);
        if (it != curr->methods.end()) {
            parent_fn = it->second;
            break;
        }
        curr = curr->parent;
    }

    if (!parent_fn) {
        throw NovaRuntimeError(
            RuntimeErrorType::NameError,
            "Method '" + method_name + "' not found in superclass hierarchy of '" + inst->klass->name + "'",
            expr.span
        );
    }

    std::vector<Value> arguments;
    for (const auto& arg_expr : expr.arguments) {
        arguments.push_back(evaluate(*arg_expr));
    }

    bool has_self_param = !parent_fn->params.empty() && parent_fn->params[0] == "self";
    std::size_t expected_args = has_self_param ? (parent_fn->params.size() - 1) : parent_fn->params.size();
    if (arguments.size() != expected_args) {
        throw NovaRuntimeError(
            RuntimeErrorType::ArgumentError,
            "Super method '" + method_name + "' expected " + std::to_string(expected_args) +
            " arguments, got " + std::to_string(arguments.size()),
            expr.span
        );
    }

    auto fn_env = Environment::create(parent_fn->closure);
    fn_env->define("self", Value(inst));
    std::size_t offset = has_self_param ? 1 : 0;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        fn_env->define(parent_fn->params[i + offset], arguments[i]);
    }

    call_stack_.push_back(CallFrame{"super." + method_name, current_filename_, expr.span});
    try {
        execute_block(parent_fn->body->statements, fn_env);
        call_stack_.pop_back();
        last_value_ = Value();
    } catch (const ReturnSignal& sig) {
        call_stack_.pop_back();
        last_value_ = sig.value;
    } catch (...) {
        call_stack_.pop_back();
        throw;
    }
}

void Interpreter::visit(ast::CallExpr& expr) {
    Value callee = evaluate(*expr.callee);
    std::vector<Value> arguments;
    for (const auto& arg_expr : expr.arguments) {
        arguments.push_back(evaluate(*arg_expr));
    }

    if (callee.is_builtin()) {
        last_value_ = callee.as_builtin()(arguments, expr.span);
        return;
    }

    if (callee.is_class()) {
        auto cls = callee.as_class();
        auto inst = std::make_shared<InstanceObject>();
        inst->klass = cls;
        inst->fields = std::make_shared<DictObject>();

        // Look for init constructor in class hierarchy
        std::shared_ptr<FunctionObject> init_fn = nullptr;
        std::shared_ptr<ClassObject> curr = cls;
        while (curr) {
            auto it = curr->methods.find("init");
            if (it != curr->methods.end()) {
                init_fn = it->second;
                break;
            }
            curr = curr->parent;
        }

        if (init_fn) {
            bool has_self_param = !init_fn->params.empty() && init_fn->params[0] == "self";
            std::size_t expected_args = has_self_param ? (init_fn->params.size() - 1) : init_fn->params.size();
            if (arguments.size() != expected_args) {
                throw NovaRuntimeError(
                    RuntimeErrorType::ArgumentError,
                    "Class '" + cls->name + "' constructor init() expected " + std::to_string(expected_args) +
                    " arguments, got " + std::to_string(arguments.size()),
                    expr.span
                );
            }

            auto fn_env = Environment::create(init_fn->closure);
            fn_env->define("self", Value(inst));
            std::size_t offset = has_self_param ? 1 : 0;
            for (std::size_t i = 0; i < arguments.size(); ++i) {
                fn_env->define(init_fn->params[i + offset], arguments[i]);
            }

            call_stack_.push_back(CallFrame{cls->name + ".init", current_filename_, expr.span});
            try {
                execute_block(init_fn->body->statements, fn_env);
                call_stack_.pop_back();
            } catch (const ReturnSignal&) {
                call_stack_.pop_back();
            } catch (...) {
                call_stack_.pop_back();
                throw;
            }
        } else if (!arguments.empty()) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "Class '" + cls->name + "' has no constructor but was called with arguments",
                expr.span
            );
        }

        last_value_ = Value(inst);
        return;
    }

    if (callee.is_function()) {
        auto fn = callee.as_function();
        if (arguments.size() != fn->params.size()) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "Function '" + fn->name + "' expected " + std::to_string(fn->params.size()) +
                " arguments, got " + std::to_string(arguments.size()),
                expr.span
            );
        }

        auto fn_env = Environment::create(fn->closure);
        for (std::size_t i = 0; i < fn->params.size(); ++i) {
            fn_env->define(fn->params[i], arguments[i]);
        }

        call_stack_.push_back(CallFrame{fn->name, current_filename_, expr.span});
        try {
            execute_block(fn->body->statements, fn_env);
            call_stack_.pop_back();
            last_value_ = Value(); // default return null
        } catch (const ReturnSignal& sig) {
            call_stack_.pop_back();
            last_value_ = sig.value;
        } catch (...) {
            call_stack_.pop_back();
            throw;
        }
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::TypeError,
        "Value of type " + std::string(callee.type_name()) + " is not callable",
        expr.span
    );
}

void Interpreter::visit(ast::ExprStmt& stmt) {
    evaluate(*stmt.expression);
}

void Interpreter::visit(ast::LetStmt& stmt) {
    Value val = stmt.initializer ? evaluate(*stmt.initializer) : Value();
    environment_->define_const(stmt.name, val);
}

void Interpreter::visit(ast::BlockStmt& stmt) {
    auto block_env = Environment::create(environment_);
    execute_block(stmt.statements, block_env);
}

void Interpreter::visit(ast::IfStmt& stmt) {
    Value condition = evaluate(*stmt.condition);
    if (condition.is_truthy()) {
        if (stmt.then_branch) {
            auto then_env = Environment::create(environment_);
            execute_block(stmt.then_branch->statements, then_env);
        }
        return;
    }

    for (const auto& elif_branch : stmt.elif_branches) {
        Value elif_cond = evaluate(*elif_branch.condition);
        if (elif_cond.is_truthy()) {
            if (elif_branch.body) {
                auto elif_env = Environment::create(environment_);
                execute_block(elif_branch.body->statements, elif_env);
            }
            return;
        }
    }

    if (stmt.else_branch) {
        auto else_env = Environment::create(environment_);
        execute_block(stmt.else_branch->statements, else_env);
    }
}

void Interpreter::visit(ast::WhileStmt& stmt) {
    std::size_t iterations = 0;
    while (evaluate(*stmt.condition).is_truthy()) {
        if (++iterations > kMaxLoopIterations) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "While loop exceeded the execution limit of " +
                                       std::to_string(kMaxLoopIterations) + " iterations", stmt.span);
        }
        if (stmt.body) {
            auto body_env = Environment::create(environment_);
            try {
                execute_block(stmt.body->statements, body_env);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
}

void Interpreter::visit(ast::ForStmt& stmt) {
    Value iterable = evaluate(*stmt.iterable);

    if (!iterable.is_array()) {
        throw NovaRuntimeError(
            RuntimeErrorType::TypeError,
            "'for' loop expects an Array or Range iterable, got " + std::string(iterable.type_name()),
            stmt.span
        );
    }

    auto arr = iterable.as_array();
    if (arr->size() > kMaxLoopIterations) {
        throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                               "For loop exceeds the execution limit of " +
                                   std::to_string(kMaxLoopIterations) + " iterations", stmt.span);
    }
    for (const auto& item : *arr) {
        auto loop_env = Environment::create(environment_);
        loop_env->define(stmt.variable_name, item);
        if (stmt.body) {
            try {
                execute_block(stmt.body->statements, loop_env);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
}

void Interpreter::visit(ast::FnDeclStmt& stmt) {
    auto fn_obj = std::make_shared<FunctionObject>();
    fn_obj->name = stmt.name;
    fn_obj->params = stmt.params;
    fn_obj->body = stmt.body;
    fn_obj->closure = environment_;

    Value fn_val(fn_obj);
    environment_->define(stmt.name, fn_val);
    last_value_ = fn_val;
}

void Interpreter::visit(ast::ReturnStmt& stmt) {
    Value val = stmt.value ? evaluate(*stmt.value) : Value();
    throw ReturnSignal{val};
}

void Interpreter::visit(ast::TryCatchStmt& stmt) {
    auto try_env = Environment::create(environment_);
    try {
        execute_block(stmt.try_branch->statements, try_env);
    } catch (const NovaRuntimeError& err) {
        auto err_dict = std::make_shared<DictObject>();
        (*err_dict)["type"] = Value(std::string(NovaRuntimeError::error_type_name(err.error_type())));
        (*err_dict)["message"] = Value(err.message());
        (*err_dict)["stack"] = Value(format_stack_trace(err.span()));

        auto catch_env = Environment::create(environment_);
        catch_env->define_const(stmt.error_var, Value(err_dict));
        if (stmt.catch_branch) {
            execute_block(stmt.catch_branch->statements, catch_env);
        }
    }
}

void Interpreter::visit(ast::GlobalStmt& stmt) {
    environment_->mark_global(stmt.name);
}

void Interpreter::visit(ast::ClassDeclStmt& stmt) {
    std::shared_ptr<ClassObject> parent_cls = nullptr;
    if (!stmt.parent_name.empty()) {
        Value parent_val = environment_->get(stmt.parent_name, stmt.span);
        if (!parent_val.is_class()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Parent '" + stmt.parent_name + "' is not a class",
                stmt.span
            );
        }
        parent_cls = parent_val.as_class();
    }

    auto cls = std::make_shared<ClassObject>();
    cls->name = stmt.name;
    cls->parent_name = stmt.parent_name;
    cls->parent = parent_cls;

    for (const auto& m : stmt.methods) {
        auto fn = std::make_shared<FunctionObject>();
        fn->name = m.name;
        fn->params = m.params;
        fn->body = m.body;
        fn->closure = environment_;

        if (m.is_static) {
            cls->statics[m.name] = Value(fn);
        } else {
            cls->methods[m.name] = fn;
        }
    }

    Value cls_val(cls);
    environment_->define(stmt.name, cls_val);
    last_value_ = cls_val;
}

void Interpreter::visit(ast::ImportStmt& stmt) {
    std::filesystem::path current_dir = std::filesystem::current_path();
    if (current_filename_ != "<main>" && current_filename_ != "<stdin>" && current_filename_ != "<repl>") {
        current_dir = std::filesystem::path(current_filename_).parent_path();
    }

    if (stmt.is_path) {
        // Direct file path (relative, absolute, or traversing outside folders)
        std::filesystem::path raw_path(stmt.module_name);
        std::filesystem::path resolved_path = raw_path.is_absolute() ? raw_path : (current_dir / raw_path);
        std::string canon_key = std::filesystem::weakly_canonical(resolved_path).string();

        auto it = module_cache_.find(canon_key);
        if (it != module_cache_.end()) {
            environment_->define(stmt.alias, Value(it->second));
            return;
        }

        if (!std::filesystem::exists(resolved_path)) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Could not find module file: " + resolved_path.string(), stmt.span);
        }

        std::ifstream file(resolved_path);
        if (!file) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Could not open module file: " + resolved_path.string(), stmt.span);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string mod_src = buffer.str();

        Lexer lexer(mod_src);
        auto tokens = lexer.tokenize();
        if (lexer.had_error()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Lexer error in module '" + stmt.module_name + "': " + lexer.error().message, stmt.span);
        }

        Parser parser(std::move(tokens));
        auto prog = parser.parse_program();
        if (parser.had_error()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Syntax error in module '" + stmt.module_name + "': " + parser.errors()[0].message, stmt.span);
        }

        auto mod_env = Environment::create(globals_);
        std::string prev_file = current_filename_;
        current_filename_ = resolved_path.string();
        try {
            execute_block(prog->statements, mod_env);
            current_filename_ = prev_file;
        } catch (...) {
            current_filename_ = prev_file;
            throw;
        }

        auto exported = mod_env->to_dict();
        module_cache_[canon_key] = exported;
        environment_->define(stmt.alias, Value(exported));
        return;
    }

    // Try built-in std module first (math, string, random, etc.)
    auto std_mod = get_or_create_std_module(stmt.module_name);
    if (std_mod) {
        environment_->define(stmt.alias, Value(std_mod));
        return;
    }

    // Check for a local .nova file with matching name
    std::filesystem::path local_file = current_dir / (stmt.module_name + ".nova");
    if (std::filesystem::exists(local_file)) {
        std::string canon_key = std::filesystem::weakly_canonical(local_file).string();
        auto it = module_cache_.find(canon_key);
        if (it != module_cache_.end()) {
            environment_->define(stmt.alias, Value(it->second));
            return;
        }

        std::ifstream file(local_file);
        if (!file) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Could not open module file: " + local_file.string(), stmt.span);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string mod_src = buffer.str();

        Lexer lexer(mod_src);
        auto tokens = lexer.tokenize();
        if (lexer.had_error()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Lexer error in module '" + stmt.module_name + "': " + lexer.error().message, stmt.span);
        }

        Parser parser(std::move(tokens));
        auto prog = parser.parse_program();
        if (parser.had_error()) {
            throw NovaRuntimeError(RuntimeErrorType::RuntimeError,
                                   "Syntax error in module '" + stmt.module_name + "': " + parser.errors()[0].message, stmt.span);
        }

        auto mod_env = Environment::create(globals_);
        std::string prev_file = current_filename_;
        current_filename_ = local_file.string();
        try {
            execute_block(prog->statements, mod_env);
            current_filename_ = prev_file;
        } catch (...) {
            current_filename_ = prev_file;
            throw;
        }

        auto exported = mod_env->to_dict();
        module_cache_[canon_key] = exported;
        environment_->define(stmt.alias, Value(exported));
        return;
    }

    throw NovaRuntimeError(
        RuntimeErrorType::RuntimeError,
        "Unknown module '" + stmt.module_name + "'",
        stmt.span
    );
}

void Interpreter::visit(ast::BreakStmt&) {
    throw BreakSignal{};
}

void Interpreter::visit(ast::ContinueStmt&) {
    throw ContinueSignal{};
}

void Interpreter::visit(ast::EnumDeclStmt& stmt) {
    auto dict = std::make_shared<DictObject>();
    for (const auto& member : stmt.members) {
        (*dict)[member] = Value(member);
    }
    Value enum_val(dict);
    environment_->define(stmt.name, enum_val);
    last_value_ = enum_val;
}

void Interpreter::visit(ast::ListComprehensionExpr& expr) {
    Value iterable = evaluate(*expr.iterable);
    if (!iterable.is_array()) {
        throw NovaRuntimeError(
            RuntimeErrorType::TypeError,
            "List comprehension expects an iterable, got " + std::string(iterable.type_name()),
            expr.span
        );
    }

    auto arr = iterable.as_array();
    auto result = std::make_shared<ListObject>();

    for (const auto& item : *arr) {
        auto comp_env = Environment::create(environment_);
        comp_env->define(expr.variable, item);

        std::shared_ptr<Environment> prev = environment_;
        environment_ = comp_env;
        try {
            bool include = true;
            if (expr.condition) {
                Value cond_val = evaluate(*expr.condition);
                include = cond_val.is_truthy();
            }
            if (include) {
                Value elem_val = evaluate(*expr.element);
                result->push_back(elem_val);
            }
            environment_ = prev;
        } catch (...) {
            environment_ = prev;
            throw;
        }
    }

    last_value_ = Value(result);
}

void Interpreter::visit(ast::Program& program) {
    for (const auto& stmt : program.statements) {
        if (stmt) {
            execute(*stmt);
        }
    }
}

}  // namespace nova
