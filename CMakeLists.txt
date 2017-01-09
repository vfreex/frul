cmake_minimum_required(VERSION 3.6)
project(frul )
set(CMAKE_C_STANDARD 99)

#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH  "$ORIGIN/../lib:$ORIGIN/")

include_directories(include)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)