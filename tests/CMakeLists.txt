add_executable(nova_test_smoke test_smoke.cpp)
target_link_libraries(nova_test_smoke PRIVATE nova_core)
target_compile_options(nova_test_smoke PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
add_test(NAME smoke COMMAND nova_test_smoke)

add_executable(nova_test_lexer test_lexer.cpp)
target_link_libraries(nova_test_lexer PRIVATE nova_core)
target_compile_options(nova_test_lexer PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
add_test(NAME lexer COMMAND nova_test_lexer)

add_executable(nova_test_parser test_parser.cpp)
target_link_libraries(nova_test_parser PRIVATE nova_core)
target_compile_options(nova_test_parser PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
add_test(NAME parser COMMAND nova_test_parser)

add_executable(nova_test_interpreter test_interpreter.cpp)
target_link_libraries(nova_test_interpreter PRIVATE nova_core)
target_compile_options(nova_test_interpreter PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
add_test(NAME interpreter COMMAND nova_test_interpreter)
