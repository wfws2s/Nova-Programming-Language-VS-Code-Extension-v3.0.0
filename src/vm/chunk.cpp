#include "vm/chunk.hpp"

#include <iomanip>
#include <iostream>

namespace nova::vm {

void Chunk::write(uint8_t byte, int line) {
    code_.push_back(byte);
    lines_.push_back(line);
}

void Chunk::write_op(OpCode op, int line) {
    write(static_cast<uint8_t>(op), line);
}

int Chunk::add_constant(Value value) {
    constants_.push_back(std::move(value));
    return static_cast<int>(constants_.size() - 1);
}

void Chunk::disassemble(const char* name) const {
    std::cout << "== " << name << " ==\n";
    for (int offset = 0; offset < static_cast<int>(code_.size());) {
        offset = disassemble_instruction(offset);
    }
}

int Chunk::disassemble_instruction(int offset) const {
    std::cout << std::setfill('0') << std::setw(4) << offset << " ";

    if (offset > 0 && lines_[offset] == lines_[offset - 1]) {
        std::cout << "   | ";
    } else {
        std::cout << std::setfill(' ') << std::setw(4) << lines_[offset] << " ";
    }

    uint8_t instruction = code_[offset];
    auto op = static_cast<OpCode>(instruction);
    std::cout << opcode_name(op);

    switch (op) {
        case OpCode::OP_CONSTANT:
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_DEFINE_GLOBAL:
        case OpCode::OP_SET_GLOBAL: {
            uint8_t constant = code_[offset + 1];
            std::cout << " " << static_cast<int>(constant) << " '";
            if (constant < constants_.size()) {
                std::cout << constants_[constant].to_string();
            }
            std::cout << "'\n";
            return offset + 2;
        }
        case OpCode::OP_GET_LOCAL:
        case OpCode::OP_SET_LOCAL:
        case OpCode::OP_CALL: {
            uint8_t slot = code_[offset + 1];
            std::cout << " " << static_cast<int>(slot) << "\n";
            return offset + 2;
        }
        case OpCode::OP_JUMP:
        case OpCode::OP_JUMP_IF_FALSE:
        case OpCode::OP_LOOP: {
            uint16_t jump = static_cast<uint16_t>((code_[offset + 1] << 8) | code_[offset + 2]);
            std::cout << " -> " << (op == OpCode::OP_LOOP ? offset + 3 - jump : offset + 3 + jump) << "\n";
            return offset + 3;
        }
        default:
            std::cout << "\n";
            return offset + 1;
    }
}

}  // namespace nova::vm
