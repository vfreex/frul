cmake_minimum_required(VERSION 3.6)
project(frul )
set(CMAKE_C_STANDARD 11)

#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(include)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)