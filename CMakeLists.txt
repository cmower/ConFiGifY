cmake_minimum_required(VERSION 3.16)
project(ConFiGifY LANGUAGES CXX)

add_library(cfgy src/cpp/cfgy.cpp)
target_include_directories(cfgy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp")
target_compile_features(cfgy PUBLIC cxx_std_17)

add_executable(cfgy-gen src/cpp/tools/generate.cpp)
target_link_libraries(cfgy-gen PRIVATE cfgy)

add_custom_command(
    OUTPUT example_config.hpp
    COMMAND cfgy-gen -o example_config.hpp --fields ExampleConfig c.myel1 myfloat2
    DEPENDS cfgy-gen
    VERBATIM
)
add_custom_target(cfgy-example-header DEPENDS example_config.hpp)

add_executable(cfgy-example examples/cpp/example.cpp)
add_dependencies(cfgy-example cfgy-example-header)
target_include_directories(cfgy-example PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(cfgy-example PRIVATE cfgy)

option(BUILD_TESTING "Build the C++ parser tests" OFF)
include(CTest)
if(BUILD_TESTING)
    set(sample "${CMAKE_CURRENT_SOURCE_DIR}/example.cfgy")
    add_custom_command(
        OUTPUT test_config.hpp
        COMMAND cfgy-gen -o test_config.hpp "${sample}" TestConfig
        DEPENDS cfgy-gen "${sample}"
        VERBATIM
    )
    add_executable(cfgy-test tests/cpp/test_configify.cpp "${CMAKE_CURRENT_BINARY_DIR}/test_config.hpp")
    add_dependencies(cfgy-test cfgy-example-header)
    target_include_directories(cfgy-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(cfgy-test PRIVATE cfgy)
    add_test(NAME cfgy-parser COMMAND cfgy-test)
    set_tests_properties(cfgy-parser PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
