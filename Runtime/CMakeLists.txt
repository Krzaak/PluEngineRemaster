project(Runtime)

add_executable(PluRuntime WIN32
        RuntimeApp.cpp
        RuntimeApp.h
        Mains/WindowsMain.cpp
        Mains/LinuxMain.cpp)

target_link_libraries(PluRuntime PUBLIC Engine)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE argparse::argparse)
find_package(Jolt CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE Jolt::Jolt)
target_compile_definitions(PluRuntime PRIVATE JPH_DEBUG_RENDERER)