add_executable(bombe main.cpp)
target_link_libraries(bombe PRIVATE bombe_core)
target_include_directories(bombe PRIVATE ${CMAKE_SOURCE_DIR}/third_party)

add_executable(generate_crib generate_crib.cpp)
target_link_libraries(generate_crib PRIVATE bombe_core)
target_include_directories(generate_crib PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
