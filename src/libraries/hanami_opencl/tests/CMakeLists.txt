cmake_minimum_required(VERSION 3.16)
project(tests VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)






add_subdirectory(functional_tests)
add_subdirectory(benchmark_tests)
