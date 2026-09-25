#pragma once

#include "runtime/value.hpp"
#include "vm/opcode.hpp"

#include <cstdint>
#include <vector>

namespace nova::vm {

class Chunk {
public:
    Chunk() = default;

    void write(uint8_t byte, int line);
    void write_op(OpCode op, int line);
    int add_constant(Value value);

    std::vector<uint8_t>& code() { return code_; }
    const std::vector<uint8_t>& code() const { return code_; }
    const std::vector<int>& lines() const { return lines_; }
    const std::vector<Value>& constants() const { return constants_; }

    void disassemble(const char* name) const;
    int disassemble_instruction(int offset) const;

private:
    std::vector<uint8_t> code_;
    std::vector<int> lines_;
    std::vector<Value> constants_;
};

}  // namespace nova::vm
