#pragma once

#include "runtime/runtime_error.hpp"
#include "runtime/value.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace nova {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> parent) : parent_(std::move(parent)) {}

    static std::shared_ptr<Environment> create() {
        return std::make_shared<Environment>();
    }

    static std::shared_ptr<Environment> create(std::shared_ptr<Environment> parent) {
        return std::make_shared<Environment>(std::move(parent));
    }

    std::shared_ptr<Environment> parent() const { return parent_; }
    std::shared_ptr<Environment> root();

    void mark_global(const std::string& name);
    bool is_global(const std::string& name) const;

    void define(const std::string& name, Value value, bool is_constant = false);
    void define_const(const std::string& name, Value value) {
        define(name, std::move(value), true);
    }
    void assign(const std::string& name, Value value, const SourceSpan& span);
    void define_or_assign(const std::string& name, Value value, const SourceSpan& span);
    Value get(const std::string& name, const SourceSpan& span) const;
    bool contains(const std::string& name) const;
    bool is_constant(const std::string& name) const;
    std::shared_ptr<DictObject> to_dict() const;

private:
    struct Symbol {
        Value value;
        bool is_constant = false;
    };

    std::shared_ptr<Environment> parent_ = nullptr;
    std::unordered_map<std::string, Symbol> values_;
    std::unordered_map<std::string, bool> globals_;
};

}  // namespace nova
