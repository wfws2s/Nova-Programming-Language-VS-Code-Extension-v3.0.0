#pragma once

#include "runtime/value.hpp"
#include "vm/chunk.hpp"

#include <unordered_map>
#include <vector>

namespace nova::vm {

enum class InterpretResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR,
};

class VM {
public:
    VM();
    ~VM() = default;

    InterpretResult interpret(const Chunk& chunk);

    void push(Value value);
    Value pop();
    Value peek(int distance = 0) const;

private:
    InterpretResult run();

    const Chunk* chunk_ = nullptr;
    const uint8_t* ip_ = nullptr;
    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
};

}  // namespace nova::vm
