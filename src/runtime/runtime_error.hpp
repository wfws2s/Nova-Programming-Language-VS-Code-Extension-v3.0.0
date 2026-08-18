#pragma once

#include "source/source_location.hpp"

#include <exception>
#include <string>
#include <utility>

namespace nova {

enum class RuntimeErrorType {
    RuntimeError,
    TypeError,
    NameError,
    ConstantError,
    KeyError,
    ArgumentError,
    IndexError
};

class NovaRuntimeError : public std::exception {
public:
    NovaRuntimeError(RuntimeErrorType error_type, std::string message, SourceSpan span, std::string hint = "")
        : error_type_(error_type), message_(std::move(message)), span_(span), hint_(std::move(hint)) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    RuntimeErrorType error_type() const { return error_type_; }
    const std::string& message() const { return message_; }
    SourceSpan span() const { return span_; }
    const std::string& hint() const { return hint_; }

    static const char* error_type_name(RuntimeErrorType type) {
        switch (type) {
            case RuntimeErrorType::TypeError: return "TypeError";
            case RuntimeErrorType::NameError: return "NameError";
            case RuntimeErrorType::ConstantError: return "ConstantError";
            case RuntimeErrorType::KeyError: return "KeyError";
            case RuntimeErrorType::ArgumentError: return "ArgumentError";
            case RuntimeErrorType::IndexError: return "IndexError";
            case RuntimeErrorType::RuntimeError: return "RuntimeError";
        }
        return "RuntimeError";
    }

private:
    RuntimeErrorType error_type_;
    std::string message_;
    SourceSpan span_;
    std::string hint_;
};

}  // namespace nova
