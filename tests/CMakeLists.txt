include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent overriding the parent project's compiler/linker settings (Windows)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(bombe_tests
    test_enigma.cpp
    test_menu.cpp
    test_bombe.cpp
)

target_link_libraries(bombe_tests
    PRIVATE bombe_core GTest::gtest_main
)

target_include_directories(bombe_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/third_party
)

include(GoogleTest)
gtest_discover_tests(bombe_tests)
