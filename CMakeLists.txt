cmake_minimum_required(VERSION 4.0.2)
project(PluEngine)

set(CMAKE_CXX_STANDARD 20)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")


add_subdirectory(Editor)
add_subdirectory(Engine)
add_subdirectory(PluSTL)
add_subdirectory(Runtime)
