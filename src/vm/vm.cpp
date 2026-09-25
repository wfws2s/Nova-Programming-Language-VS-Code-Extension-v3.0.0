#include "vm/vm.hpp"

#include <cmath>
#include <iostream>

namespace nova::vm {

VM::VM() {
    stack_.reserve(256);
}

void VM::push(Value value) {
    stack_.push_back(std::move(value));
}

Value VM::pop() {
    if (stack_.empty()) return Value();
    Value val = std::move(stack_.back());
    stack_.pop_back();
    return val;
}

Value VM::peek(int distance) const {
    if (distance >= static_cast<int>(stack_.size())) return Value();
    return stack_[stack_.size() - 1 - distance];
}

InterpretResult VM::interpret(const Chunk& chunk) {
    chunk_ = &chunk;
    ip_ = chunk_->code().data();
    return run();
}

InterpretResult VM::run() {
#define READ_BYTE() (*ip_++)
#define READ_SHORT() (ip_ += 2, static_cast<uint16_t>((ip_[-2] << 8) | ip_[-1]))
#define READ_CONSTANT() (chunk_->constants()[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
        if (!peek(0).is_number() || !peek(1).is_number()) { \
            std::cerr << "Operands must be numbers.\n"; \
            return InterpretResult::RUNTIME_ERROR; \
        } \
        double b = pop().as_number(); \
        double a = pop().as_number(); \
        push(Value(a op b)); \
    } while (false)

    while (true) {
        uint8_t instruction = READ_BYTE();
        auto op = static_cast<OpCode>(instruction);

        switch (op) {
            case OpCode::OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OpCode::OP_NIL: push(Value()); break;
            case OpCode::OP_TRUE: push(Value(true)); break;
            case OpCode::OP_FALSE: push(Value(false)); break;
            case OpCode::OP_POP: pop(); break;

            case OpCode::OP_GET_GLOBAL: {
                Value name_val = READ_CONSTANT();
                const std::string& name = name_val.as_string();
                auto it = globals_.find(name);
                if (it == globals_.end()) {
                    std::cerr << "Undefined variable '" << name << "'.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                push(it->second);
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL: {
                Value name_val = READ_CONSTANT();
                globals_[name_val.as_string()] = peek(0);
                pop();
                break;
            }
            case OpCode::OP_SET_GLOBAL: {
                Value name_val = READ_CONSTANT();
                const std::string& name = name_val.as_string();
                auto it = globals_.find(name);
                if (it == globals_.end()) {
                    std::cerr << "Undefined variable '" << name << "'.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                it->second = peek(0);
                break;
            }

            case OpCode::OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::OP_GREATER: BINARY_OP(>); break;
            case OpCode::OP_LESS: BINARY_OP(<); break;
            case OpCode::OP_ADD: {
                if (peek(0).is_string() && peek(1).is_string()) {
                    std::string b = pop().as_string();
                    std::string a = pop().as_string();
                    push(Value(a + b));
                } else if (peek(0).is_number() && peek(1).is_number()) {
                    double b = pop().as_number();
                    double a = pop().as_number();
                    push(Value(a + b));
                } else {
                    std::cerr << "Operands must be two numbers or two strings.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::OP_SUBTRACT: BINARY_OP(-); break;
            case OpCode::OP_MULTIPLY: BINARY_OP(*); break;
            case OpCode::OP_DIVIDE: BINARY_OP(/); break;
            case OpCode::OP_MODULO: {
                if (!peek(0).is_number() || !peek(1).is_number()) {
                    std::cerr << "Operands must be numbers.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(Value(std::fmod(a, b)));
                break;
            }
            case OpCode::OP_POWER: {
                if (!peek(0).is_number() || !peek(1).is_number()) {
                    std::cerr << "Operands must be numbers.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(Value(std::pow(a, b)));
                break;
            }
            case OpCode::OP_NOT: push(Value(!pop().is_truthy())); break;
            case OpCode::OP_NEGATE: {
                if (!peek(0).is_number()) {
                    std::cerr << "Operand must be a number.\n";
                    return InterpretResult::RUNTIME_ERROR;
                }
                push(Value(-pop().as_number()));
                break;
            }
            case OpCode::OP_PRINT: {
                std::cout << pop().to_string() << "\n";
                break;
            }
            case OpCode::OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip_ += offset;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!peek(0).is_truthy()) ip_ += offset;
                break;
            }
            case OpCode::OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip_ -= offset;
                break;
            }
            case OpCode::OP_RETURN: {
                return InterpretResult::OK;
            }
            default:
                std::cerr << "Unknown opcode " << static_cast<int>(instruction) << "\n";
                return InterpretResult::RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
}

}  // namespace nova::vm
