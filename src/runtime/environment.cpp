#include "runtime/environment.hpp"

namespace nova {

std::shared_ptr<Environment> Environment::root() {
    std::shared_ptr<Environment> curr = shared_from_this();
    while (curr->parent_) {
        curr = curr->parent_;
    }
    return curr;
}

void Environment::mark_global(const std::string& name) {
    globals_[name] = true;
}

bool Environment::is_global(const std::string& name) const {
    auto it = globals_.find(name);
    if (it != globals_.end() && it->second) {
        return true;
    }
    if (parent_) {
        return parent_->is_global(name);
    }
    return false;
}

void Environment::define(const std::string& name, Value value, bool is_constant) {
    if (is_global(name) && parent_) {
        root()->define(name, std::move(value), is_constant);
        return;
    }
    values_[name] = Symbol{std::move(value), is_constant};
}

void Environment::assign(const std::string& name, Value value, const SourceSpan& span) {
    if (is_global(name) && parent_) {
        root()->assign(name, std::move(value), span);
        return;
    }

    auto it = values_.find(name);
    if (it != values_.end()) {
        if (it->second.is_constant) {
            throw NovaRuntimeError(
                RuntimeErrorType::ConstantError,
                "cannot reassign constant '" + name + "'",
                span,
                "'" + name + "' was declared as a constant with 'let " + name + " = ...'."
            );
        }
        it->second.value = std::move(value);
        return;
    }

    if (parent_) {
        // If parent contains the variable (mutable or constant)
        if (parent_->contains(name)) {
            parent_->assign(name, std::move(value), span);
            return;
        }
    }

    // Normal assignment declares a new mutable variable if not previously declared
    values_[name] = Symbol{std::move(value), false};
}

void Environment::define_or_assign(const std::string& name, Value value, const SourceSpan& span) {
    assign(name, std::move(value), span);
}

Value Environment::get(const std::string& name, const SourceSpan& span) const {
    if (is_global(name) && parent_) {
        auto r = const_cast<Environment*>(this)->root();
        return r->get(name, span);
    }

    auto it = values_.find(name);
    if (it != values_.end()) {
        return it->second.value;
    }

    if (parent_) {
        return parent_->get(name, span);
    }

    throw NovaRuntimeError(
        RuntimeErrorType::NameError,
        "Undefined variable '" + name + "'",
        span,
        "Check variable spelling or declare with '" + name + " = ...' or 'let " + name + " = ...'"
    );
}

bool Environment::contains(const std::string& name) const {
    if (values_.find(name) != values_.end()) {
        return true;
    }
    if (parent_) {
        return parent_->contains(name);
    }
    return false;
}

bool Environment::is_constant(const std::string& name) const {
    auto it = values_.find(name);
    if (it != values_.end()) {
        return it->second.is_constant;
    }
    if (parent_) {
        return parent_->is_constant(name);
    }
    return false;
}

std::shared_ptr<DictObject> Environment::to_dict() const {
    auto dict = std::make_shared<DictObject>();
    for (const auto& [name, sym] : values_) {
        (*dict)[name] = sym.value;
    }
    return dict;
}

}  // namespace nova
