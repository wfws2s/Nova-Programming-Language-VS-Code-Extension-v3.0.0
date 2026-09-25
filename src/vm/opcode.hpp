#pragma once

#include <cstdint>
#include <string_view>

namespace nova::vm {

enum class OpCode : uint8_t {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POWER,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_RETURN,
};

constexpr std::string_view opcode_name(OpCode op) {
    switch (op) {
        case OpCode::OP_CONSTANT: return "OP_CONSTANT";
        case OpCode::OP_NIL: return "OP_NIL";
        case OpCode::OP_TRUE: return "OP_TRUE";
        case OpCode::OP_FALSE: return "OP_FALSE";
        case OpCode::OP_POP: return "OP_POP";
        case OpCode::OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OpCode::OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
        case OpCode::OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OpCode::OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OpCode::OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OpCode::OP_EQUAL: return "OP_EQUAL";
        case OpCode::OP_GREATER: return "OP_GREATER";
        case OpCode::OP_LESS: return "OP_LESS";
        case OpCode::OP_ADD: return "OP_ADD";
        case OpCode::OP_SUBTRACT: return "OP_SUBTRACT";
        case OpCode::OP_MULTIPLY: return "OP_MULTIPLY";
        case OpCode::OP_DIVIDE: return "OP_DIVIDE";
        case OpCode::OP_MODULO: return "OP_MODULO";
        case OpCode::OP_POWER: return "OP_POWER";
        case OpCode::OP_NOT: return "OP_NOT";
        case OpCode::OP_NEGATE: return "OP_NEGATE";
        case OpCode::OP_PRINT: return "OP_PRINT";
        case OpCode::OP_JUMP: return "OP_JUMP";
        case OpCode::OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OpCode::OP_LOOP: return "OP_LOOP";
        case OpCode::OP_CALL: return "OP_CALL";
        case OpCode::OP_CLOSURE: return "OP_CLOSURE";
        case OpCode::OP_RETURN: return "OP_RETURN";
    }
    return "UNKNOWN_OP";
}

}  // namespace nova::vm
