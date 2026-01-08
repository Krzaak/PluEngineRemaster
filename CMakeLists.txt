cmake_minimum_required(VERSION 4.0.2)
project(PluEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")

if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_WARNINGS
    )
endif()


add_subdirectory(Editor)
add_subdirectory(Engine)
add_subdirectory(PluSTL)
add_subdirectory(Runtime)
