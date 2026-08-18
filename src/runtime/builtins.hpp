#pragma once

#include "runtime/environment.hpp"

#include <iostream>
#include <memory>

namespace nova {

class Builtins {
public:
    static void register_all(Environment& env, std::ostream& out = std::cout,
                             std::istream& in = std::cin);
};

}  // namespace nova
