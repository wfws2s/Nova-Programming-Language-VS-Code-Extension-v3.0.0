#include "../src/version.hpp"

#include "test_helpers.hpp"

int main() {
    nova::test::expect_eq(NOVA_VERSION_STRING, "0.2.2", "version string");
    nova::test::expect_true(NOVA_VERSION_MAJOR == 0, "major version");
    nova::test::expect_true(NOVA_VERSION_MINOR == 2, "minor version");
    nova::test::expect_true(NOVA_VERSION_PATCH == 2, "patch version");

    return nova::test::finish("test_smoke");
}
