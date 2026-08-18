#pragma once

#include "source/source_location.hpp"

#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace nova {

namespace ast {
class BlockStmt;
}

class Environment;

enum class ValueType {
    Null,
    Boolean,
    Number,
    String,
    List,
    Dictionary,
    Function,
    BuiltinFunction,
    Class,
    Instance,
};

class Value;
using ListObject = std::vector<Value>;
using ArrayObject = ListObject;
using DictObject = std::unordered_map<std::string, Value>;

struct FunctionObject {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<ast::BlockStmt> body;
    std::shared_ptr<Environment> closure;
};

using BuiltinFn = std::function<Value(const std::vector<Value>& args, const SourceSpan& span)>;

// Represents a class definition
struct ClassObject {
    std::string name;
    std::string parent_name;
    std::shared_ptr<ClassObject> parent; // resolved at runtime

    // Instance methods (name -> FunctionObject)
    std::unordered_map<std::string, std::shared_ptr<FunctionObject>> methods;
    // Static methods/fields (name -> Value)
    std::unordered_map<std::string, Value> statics;
};

// Represents a live instance of a class
struct InstanceObject {
    std::shared_ptr<ClassObject> klass;
    std::shared_ptr<DictObject> fields; // self.x lives here
};

class Value {
public:
    Value(); // Null
    Value(bool b);
    Value(double n);
    Value(std::string s);
    Value(const char* s);
    Value(std::shared_ptr<ListObject> list);
    Value(std::shared_ptr<DictObject> dict);
    Value(std::shared_ptr<FunctionObject> fn);
    Value(BuiltinFn fn, std::string name = "<builtin>");
    Value(std::shared_ptr<ClassObject> cls);
    Value(std::shared_ptr<InstanceObject> inst);

    ValueType type() const;
    const char* type_name() const;

    bool is_null() const { return type() == ValueType::Null; }
    bool is_bool() const { return type() == ValueType::Boolean; }
    bool is_number() const { return type() == ValueType::Number; }
    bool is_string() const { return type() == ValueType::String; }
    bool is_list() const { return type() == ValueType::List; }
    bool is_array() const { return is_list(); }
    bool is_dict() const { return type() == ValueType::Dictionary; }
    bool is_function() const { return type() == ValueType::Function; }
    bool is_builtin() const { return type() == ValueType::BuiltinFunction; }
    bool is_class() const { return type() == ValueType::Class; }
    bool is_instance() const { return type() == ValueType::Instance; }

    bool as_bool() const;
    double as_number() const;
    const std::string& as_string() const;
    std::shared_ptr<ListObject> as_list() const;
    std::shared_ptr<ArrayObject> as_array() const { return as_list(); }
    std::shared_ptr<DictObject> as_dict() const;
    std::shared_ptr<FunctionObject> as_function() const;
    const BuiltinFn& as_builtin() const;
    std::shared_ptr<ClassObject> as_class() const;
    std::shared_ptr<InstanceObject> as_instance() const;

    bool is_truthy() const;
    bool equals(const Value& other) const;
    std::string to_string() const;

    bool operator==(const Value& other) const {
        return equals(other);
    }

    bool operator!=(const Value& other) const {
        return !equals(other);
    }

private:
    struct BuiltinWrapper {
        BuiltinFn func;
        std::string name;
    };

    std::variant<
        std::monostate,
        bool,
        double,
        std::string,
        std::shared_ptr<ListObject>,
        std::shared_ptr<DictObject>,
        std::shared_ptr<FunctionObject>,
        BuiltinWrapper,
        std::shared_ptr<ClassObject>,
        std::shared_ptr<InstanceObject>
    > data_;
};

std::ostream& operator<<(std::ostream& os, const Value& val);

}  // namespace nova
