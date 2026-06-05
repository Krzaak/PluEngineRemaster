project(Runtime)

file(GLOB_RECURSE SOURCES_REFLECTION CONFIGURE_DEPENDS "../ReflectionCache/Runtime/*.cpp")

add_executable(PluRuntime WIN32 ${SOURCES_REFLECTION}
        RuntimeApp.cpp
        RuntimeApp.h
        Mains/WindowsMain.cpp
        Mains/LinuxMain.cpp
        Shaders/RuntimeShaderManager.h
        Shaders/RuntimeShaderManager.cpp
        Shaders/RuntimeShaderCode.cpp
        Shaders/RuntimeShaderCode.h)

target_link_libraries(PluRuntime PUBLIC Engine)
set_target_properties(PluRuntime PROPERTIES UNITY_BUILD ON)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE argparse::argparse)
find_package(Jolt CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE Jolt::Jolt)
target_compile_definitions(PluRuntime PRIVATE JPH_DEBUG_RENDERER)

target_include_directories(PluRuntime PUBLIC ../ReflectionCache/Runtime)

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