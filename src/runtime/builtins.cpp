#include "runtime/builtins.hpp"
#include "runtime/runtime_error.hpp"

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace nova {

namespace {

std::string trim_string(std::string_view s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return std::string(s.substr(start, end - start));
}

class LiteralParser {
public:
    explicit LiteralParser(std::string_view source) : src_(source) {}

    bool parse_value(Value& out) {
        skip_whitespace();
        if (pos_ >= src_.size()) {
            return false;
        }

        if (match("null")) {
            out = Value();
            return true;
        }
        if (match("true")) {
            out = Value(true);
            return true;
        }
        if (match("false")) {
            out = Value(false);
            return true;
        }

        char c = src_[pos_];
        if (c == '"') {
            return parse_string(out);
        }
        if (c == '[') {
            return parse_list(out);
        }
        if (c == '{') {
            return parse_dict(out);
        }
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '-' && pos_ + 1 < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_ + 1])) || src_[pos_ + 1] == '.'))) {
            return parse_number(out);
        }

        return false;
    }

    bool parse_complete(Value& out) {
        if (!parse_value(out)) return false;
        skip_whitespace();
        return pos_ == src_.size();
    }

private:
    void skip_whitespace() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
    }

    bool match(std::string_view expected) {
        if (src_.substr(pos_, expected.size()) == expected) {
            if (pos_ + expected.size() < src_.size()) {
                char next = src_[pos_ + expected.size()];
                if (std::isalnum(static_cast<unsigned char>(next)) || next == '_') {
                    return false;
                }
            }
            pos_ += expected.size();
            return true;
        }
        return false;
    }

    bool parse_number(Value& out) {
        std::size_t start = pos_;
        if (src_[pos_] == '-') {
            ++pos_;
        }
        bool has_digits = false;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            has_digits = true;
            ++pos_;
        }
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                has_digits = true;
                ++pos_;
            }
        }
        if (!has_digits) return false;

        std::string num_str(src_.substr(start, pos_ - start));
        try {
            std::size_t idx = 0;
            double val = std::stod(num_str, &idx);
            if (idx == num_str.size()) {
                out = Value(val);
                return true;
            }
        } catch (...) {}
        return false;
    }

    bool parse_string(Value& out) {
        if (pos_ >= src_.size() || src_[pos_] != '"') return false;
        ++pos_; // skip '"'
        std::string val;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            char c = src_[pos_++];
            if (c == '\\') {
                if (pos_ >= src_.size()) return false;
                char esc = src_[pos_++];
                switch (esc) {
                    case 'n': val.push_back('\n'); break;
                    case 't': val.push_back('\t'); break;
                    case 'r': val.push_back('\r'); break;
                    case '\\': val.push_back('\\'); break;
                    case '"': val.push_back('"'); break;
                    default: return false;
                }
            } else {
                val.push_back(c);
            }
        }
        if (pos_ >= src_.size() || src_[pos_] != '"') return false;
        ++pos_; // consume '"'
        out = Value(std::move(val));
        return true;
    }

    bool parse_list(Value& out) {
        if (pos_ >= src_.size() || src_[pos_] != '[') return false;
        ++pos_; // skip '['
        auto list = std::make_shared<ListObject>();
        skip_whitespace();

        if (pos_ < src_.size() && src_[pos_] == ']') {
            ++pos_;
            out = Value(list);
            return true;
        }

        while (pos_ < src_.size()) {
            Value item;
            if (!parse_value(item)) return false;
            list->push_back(std::move(item));
            skip_whitespace();
            if (pos_ < src_.size() && src_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
                if (pos_ < src_.size() && src_[pos_] == ']') {
                    ++pos_;
                    out = Value(list);
                    return true;
                }
            } else if (pos_ < src_.size() && src_[pos_] == ']') {
                ++pos_;
                out = Value(list);
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parse_dict(Value& out) {
        if (pos_ >= src_.size() || src_[pos_] != '{') return false;
        ++pos_; // skip '{'
        auto dict = std::make_shared<DictObject>();
        skip_whitespace();

        if (pos_ < src_.size() && src_[pos_] == '}') {
            ++pos_;
            out = Value(dict);
            return true;
        }

        while (pos_ < src_.size()) {
            skip_whitespace();
            Value key_val;
            if (!parse_string(key_val)) return false;
            skip_whitespace();
            if (pos_ >= src_.size() || src_[pos_] != ':') return false;
            ++pos_; // skip ':'
            skip_whitespace();
            Value val;
            if (!parse_value(val)) return false;
            (*dict)[key_val.as_string()] = std::move(val);
            skip_whitespace();

            if (pos_ < src_.size() && src_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
                if (pos_ < src_.size() && src_[pos_] == '}') {
                    ++pos_;
                    out = Value(dict);
                    return true;
                }
            } else if (pos_ < src_.size() && src_[pos_] == '}') {
                ++pos_;
                out = Value(dict);
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    std::string_view src_;
    std::size_t pos_ = 0;
};

Value parse_literal_or_string(const std::string& input) {
    std::string trimmed = trim_string(input);
    if (trimmed.empty()) {
        return Value(input);
    }
    LiteralParser parser(trimmed);
    Value parsed;
    if (parser.parse_complete(parsed)) {
        return parsed;
    }
    return Value(input);
}

} // namespace

void Builtins::register_all(Environment& env, std::ostream& out, std::istream& in) {
    // print(...)
    env.define("print", Value([&out](const std::vector<Value>& args, const SourceSpan&) -> Value {
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i > 0) out << ' ';
            out << args[i].to_string();
        }
        out << '\n';
        return Value(); // returns null
    }, "print"));

    // input([prompt]) - Dynamic literal input parser
    env.define("input", Value([&out, &in](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() > 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "input() takes at most 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (args.size() == 1) {
            if (!args[0].is_string()) {
                throw NovaRuntimeError(
                    RuntimeErrorType::TypeError,
                    "input() prompt must be a String, got " + std::string(args[0].type_name()),
                    span
                );
            }
            out << args[0].as_string();
            out.flush();
        }
        std::string line;
        if (!std::getline(in, line)) {
            return Value("");
        }
        return parse_literal_or_string(line);
    }, "input"));

    // read([prompt]) - Dynamic literal input parser
    env.define("read", Value([&out, &in](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() > 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "read() takes at most 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (args.size() == 1) {
            if (!args[0].is_string()) {
                throw NovaRuntimeError(
                    RuntimeErrorType::TypeError,
                    "read() prompt must be a String, got " + std::string(args[0].type_name()),
                    span
                );
            }
            out << args[0].as_string();
            out.flush();
        }
        std::string line;
        if (!std::getline(in, line)) {
            return Value("");
        }
        return parse_literal_or_string(line);
    }, "read"));

    // len(val)
    env.define("len", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "len() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        const Value& val = args[0];
        if (val.is_string()) {
            return Value(static_cast<double>(val.as_string().size()));
        }
        if (val.is_array()) {
            return Value(static_cast<double>(val.as_array()->size()));
        }
        if (val.is_dict()) {
            return Value(static_cast<double>(val.as_dict()->size()));
        }
        throw NovaRuntimeError(
            RuntimeErrorType::TypeError,
            "len() argument must be a String, List, or Dictionary, not " + std::string(val.type_name()),
            span
        );
    }, "len"));

    // number(val)
    env.define("number", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "number() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        const Value& val = args[0];
        if (val.is_number()) {
            return val;
        }
        if (val.is_bool()) {
            return Value(val.as_bool() ? 1.0 : 0.0);
        }
        if (val.is_string()) {
            try {
                std::size_t idx = 0;
                double res = std::stod(val.as_string(), &idx);
                if (idx == val.as_string().size()) {
                    return Value(res);
                }
            } catch (...) {}
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "Cannot parse string \"" + val.as_string() + "\" as Number",
                span,
                "Ensure string contains a valid numeric format."
            );
        }
        throw NovaRuntimeError(
            RuntimeErrorType::TypeError,
            "Cannot convert " + std::string(val.type_name()) + " to Number",
            span
        );
    }, "number"));

    // string(val)
    env.define("string", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "string() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        return Value(args[0].to_string());
    }, "string"));

    // typeof(val) - Full release canonical type name
    env.define("typeof", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "typeof() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        return Value(std::string(args[0].type_name()));
    }, "typeof"));

    // type(val) - Alias for typeof
    env.define("type", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "type() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        return Value(std::string(args[0].type_name()));
    }, "type"));

    // list(val) - Full release explicit list conversion
    env.define("list", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "list() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        const Value& val = args[0];
        if (val.is_list()) {
            auto src = val.as_list();
            auto copy = std::make_shared<ListObject>(*src);
            return Value(copy);
        }
        if (val.is_string()) {
            auto list = std::make_shared<ListObject>();
            for (char c : val.as_string()) {
                list->push_back(Value(std::string(1, c)));
            }
            return Value(list);
        }
        if (val.is_dict()) {
            auto dict = val.as_dict();
            auto list = std::make_shared<ListObject>();
            for (const auto& [k, v] : *dict) {
                list->push_back(Value(k));
            }
            return Value(list);
        }
        if (val.is_null()) {
            return Value(std::make_shared<ListObject>());
        }
        auto list = std::make_shared<ListObject>();
        list->push_back(val);
        return Value(list);
    }, "list"));

    // split(text, delimiter) - Full release string splitter
    env.define("split", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 2) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "split() takes exactly 2 arguments (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (!args[0].is_string()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "split() first argument must be a String, got " + std::string(args[0].type_name()),
                span
            );
        }
        if (!args[1].is_string()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "split() second argument must be a String, got " + std::string(args[1].type_name()),
                span
            );
        }
        const std::string& text = args[0].as_string();
        const std::string& delim = args[1].as_string();
        if (delim.empty()) {
            throw NovaRuntimeError(
                RuntimeErrorType::RuntimeError,
                "split() delimiter cannot be empty",
                span
            );
        }

        auto list = std::make_shared<ListObject>();
        std::size_t start = 0;
        while (start <= text.size()) {
            std::size_t end = text.find(delim, start);
            if (end == std::string::npos) {
                list->push_back(Value(text.substr(start)));
                break;
            }
            list->push_back(Value(text.substr(start, end - start)));
            start = end + delim.size();
        }
        return Value(list);
    }, "split"));

    // append(list, value) - Full release in-place append
    env.define("append", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 2) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "append() takes exactly 2 arguments (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (!args[0].is_list()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "append() first argument must be a List, got " + std::string(args[0].type_name()),
                span
            );
        }
        args[0].as_list()->push_back(args[1]);
        return Value(); // returns null
    }, "append"));

    // pop(list) - Full release removal of final item
    env.define("pop", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "pop() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (!args[0].is_list()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "pop() argument must be a List, got " + std::string(args[0].type_name()),
                span
            );
        }
        auto list = args[0].as_list();
        if (list->empty()) {
            throw NovaRuntimeError(
                RuntimeErrorType::IndexError,
                "Cannot pop from an empty List",
                span
            );
        }
        Value last = list->back();
        list->pop_back();
        return last;
    }, "pop"));

    // keys(dict) - Full release dictionary keys list
    env.define("keys", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "keys() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (!args[0].is_dict()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "keys() argument must be a Dictionary, got " + std::string(args[0].type_name()),
                span
            );
        }
        auto dict = args[0].as_dict();
        auto list = std::make_shared<ListObject>();
        for (const auto& [k, v] : *dict) {
            list->push_back(Value(k));
        }
        return Value(list);
    }, "keys"));

    // values(dict) - Full release dictionary values list
    env.define("values", Value([](const std::vector<Value>& args, const SourceSpan& span) -> Value {
        if (args.size() != 1) {
            throw NovaRuntimeError(
                RuntimeErrorType::ArgumentError,
                "values() takes exactly 1 argument (" + std::to_string(args.size()) + " given)",
                span
            );
        }
        if (!args[0].is_dict()) {
            throw NovaRuntimeError(
                RuntimeErrorType::TypeError,
                "values() argument must be a Dictionary, got " + std::string(args[0].type_name()),
                span
            );
        }
        auto dict = args[0].as_dict();
        auto list = std::make_shared<ListObject>();
        for (const auto& [k, v] : *dict) {
            list->push_back(v);
        }
        return Value(list);
    }, "values"));
}

}  // namespace nova
