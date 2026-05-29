add_library(bombe_core STATIC
    src/enigma.cpp
    src/menu.cpp
    src/bombe.cpp
)

target_include_directories(bombe_core
    PUBLIC  include
    PRIVATE ${CMAKE_SOURCE_DIR}/third_party
)

target_compile_features(bombe_core PUBLIC cxx_std_17)
