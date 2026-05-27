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

if(WIN32)
    add_custom_command(TARGET PluRuntime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
            $<TARGET_FILE_DIR:PluRuntime>    # To weźmie folder, gdzie powstaje PluEditor.exe
    )
endif()

add_custom_command(TARGET PluRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/ReflectionCache/PluEngine.pyi
        $<TARGET_FILE_DIR:PluRuntime>
)