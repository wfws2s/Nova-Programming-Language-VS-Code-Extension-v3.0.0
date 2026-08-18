#include "runtime/value.hpp"

#include <cmath>
#include <functional>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <unordered_set>

namespace nova {

Value::Value() : data_(std::monostate{}) {}
Value::Value(bool b) : data_(b) {}
Value::Value(double n) : data_(n) {}
Value::Value(std::string s) : data_(std::move(s)) {}
Value::Value(const char* s) : data_(std::string(s)) {}
Value::Value(std::shared_ptr<ListObject> list) : data_(std::move(list)) {}
Value::Value(std::shared_ptr<DictObject> dict) : data_(std::move(dict)) {}
Value::Value(std::shared_ptr<FunctionObject> fn) : data_(std::move(fn)) {}
Value::Value(BuiltinFn fn, std::string name) : data_(BuiltinWrapper{std::move(fn), std::move(name)}) {}
Value::Value(std::shared_ptr<ClassObject> cls) : data_(std::move(cls)) {}
Value::Value(std::shared_ptr<InstanceObject> inst) : data_(std::move(inst)) {}

ValueType Value::type() const {
    switch (data_.index()) {
        case 0: return ValueType::Null;
        case 1: return ValueType::Boolean;
        case 2: return ValueType::Number;
        case 3: return ValueType::String;
        case 4: return ValueType::List;
        case 5: return ValueType::Dictionary;
        case 6: return ValueType::Function;
        case 7: return ValueType::BuiltinFunction;
        case 8: return ValueType::Class;
        case 9: return ValueType::Instance;
    }
    return ValueType::Null;
}

const char* Value::type_name() const {
    switch (type()) {
        case ValueType::Null: return "Null";
        case ValueType::Boolean: return "Boolean";
        case ValueType::Number: return "Number";
        case ValueType::String: return "String";
        case ValueType::List: return "List";
        case ValueType::Dictionary: return "Dictionary";
        case ValueType::Function: return "Function";
        case ValueType::BuiltinFunction: return "Function";
        case ValueType::Class: return "Class";
        case ValueType::Instance: return "Instance";
    }
    return "Unknown";
}

bool Value::as_bool() const {
    return std::get<bool>(data_);
}

double Value::as_number() const {
    return std::get<double>(data_);
}

const std::string& Value::as_string() const {
    return std::get<std::string>(data_);
}

std::shared_ptr<ListObject> Value::as_list() const {
    return std::get<std::shared_ptr<ListObject>>(data_);
}

std::shared_ptr<DictObject> Value::as_dict() const {
    return std::get<std::shared_ptr<DictObject>>(data_);
}

std::shared_ptr<FunctionObject> Value::as_function() const {
    return std::get<std::shared_ptr<FunctionObject>>(data_);
}

const BuiltinFn& Value::as_builtin() const {
    return std::get<BuiltinWrapper>(data_).func;
}

std::shared_ptr<ClassObject> Value::as_class() const {
    return std::get<std::shared_ptr<ClassObject>>(data_);
}

std::shared_ptr<InstanceObject> Value::as_instance() const {
    return std::get<std::shared_ptr<InstanceObject>>(data_);
}

bool Value::is_truthy() const {
    if (is_null()) {
        return false;
    }
    if (is_bool()) {
        return as_bool();
    }
    return true;
}

bool Value::equals(const Value& other) const {
    if (type() != other.type()) {
        return false;
    }

    switch (type()) {
        case ValueType::Null:
            return true;
        case ValueType::Boolean:
            return as_bool() == other.as_bool();
        case ValueType::Number:
            return as_number() == other.as_number();
        case ValueType::String:
            return as_string() == other.as_string();
        case ValueType::List: {
            auto a = as_list();
            auto b = other.as_list();
            if (a == b) return true;
            if (!a || !b || a->size() != b->size()) return false;
            for (std::size_t i = 0; i < a->size(); ++i) {
                if (!(*a)[i].equals((*b)[i])) {
                    return false;
                }
            }
            return true;
        }
        case ValueType::Dictionary: {
            auto a = as_dict();
            auto b = other.as_dict();
            if (a == b) return true;
            if (!a || !b || a->size() != b->size()) return false;
            for (const auto& [k, v] : *a) {
                auto it = b->find(k);
                if (it == b->end() || !v.equals(it->second)) {
                    return false;
                }
            }
            return true;
        }
        case ValueType::Function:
            return as_function() == other.as_function();
        case ValueType::BuiltinFunction:
            return false;
        case ValueType::Class:
            return as_class() == other.as_class();
        case ValueType::Instance:
            return as_instance() == other.as_instance();
    }
    return false;
}

std::string Value::to_string() const {
    std::unordered_set<const void*> active_collections;
    std::function<std::string(const Value&)> format = [&](const Value& value) -> std::string {
        switch (value.type()) {
            case ValueType::Null:
                return "null";
            case ValueType::Boolean:
                return value.as_bool() ? "true" : "false";
            case ValueType::Number: {
                double number = value.as_number();
                if (std::isnan(number)) return "NaN";
                if (std::isinf(number)) return number > 0 ? "Infinity" : "-Infinity";
                if (std::trunc(number) == number && std::abs(number) < 1e15) {
                    return std::to_string(static_cast<long long>(number));
                }
                std::ostringstream ss;
                ss << std::setprecision(14) << number;
                return ss.str();
            }
            case ValueType::String:
                return value.as_string();
            case ValueType::List: {
                auto list = value.as_list();
                if (!list) return "[]";
                if (!active_collections.insert(list.get()).second) return "<cycle>";
                std::string result = "[";
                for (std::size_t i = 0; i < list->size(); ++i) {
                    if (i > 0) result += ", ";
                    result += (*list)[i].is_string() ? "\"" + (*list)[i].as_string() + "\"" : format((*list)[i]);
                }
                result += "]";
                active_collections.erase(list.get());
                return result;
            }
            case ValueType::Dictionary: {
                auto dictionary = value.as_dict();
                if (!dictionary) return "{}";
                if (!active_collections.insert(dictionary.get()).second) return "<cycle>";
                std::string result = "{";
                bool first = true;
                for (const auto& [key, entry] : *dictionary) {
                    if (!first) result += ", ";
                    first = false;
                    result += "\"" + key + "\": ";
                    result += entry.is_string() ? "\"" + entry.as_string() + "\"" : format(entry);
                }
                result += "}";
                active_collections.erase(dictionary.get());
                return result;
            }
            case ValueType::Function: {
                auto function = value.as_function();
                return function && !function->name.empty() ? "<fn " + function->name + ">" : "<fn>";
            }
            case ValueType::BuiltinFunction:
                return "<builtin " + std::get<BuiltinWrapper>(value.data_).name + ">";
            case ValueType::Class: {
                auto cls = value.as_class();
                return cls ? "<class " + cls->name + ">" : "<class>";
            }
            case ValueType::Instance: {
                auto inst = value.as_instance();
                if (!inst) return "<instance>";
                std::string class_name = inst->klass ? inst->klass->name : "?";
                // Show fields similar to a dict
                auto& fields = inst->fields;
                if (!fields || fields->empty()) return "<" + class_name + " instance>";
                if (!active_collections.insert(fields.get()).second) return "<cycle>";
                std::string result = "<" + class_name + " {";
                bool first = true;
                for (const auto& [key, entry] : *fields) {
                    if (!first) result += ", ";
                    first = false;
                    result += key + ": ";
                    result += entry.is_string() ? "\"" + entry.as_string() + "\"" : format(entry);
                }
                result += "}>";
                active_collections.erase(fields.get());
                return result;
            }
        }
        return "null";
    };
    return format(*this);
}

std::ostream& operator<<(std::ostream& os, const Value& val) {
    return os << val.to_string();
}

}  // namespace nova
