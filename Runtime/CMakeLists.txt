project(Runtime)

# Bootstrap reflection sources on the first configure so the GLOB below has files to
# collect. On later builds the PRE_BUILD command keeps them regenerated, so we skip the
# configure-time run to avoid generating twice whenever CMake reconfigures.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ReflectionCache/Runtime")
    execute_process(
            COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Runtime
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

file(GLOB_RECURSE SOURCES_REFLECTION CONFIGURE_DEPENDS "../ReflectionCache/Runtime/*.cpp")

add_executable(PluRuntime WIN32 ${SOURCES_REFLECTION}
        RuntimeApp.cpp
        RuntimeApp.h
        Mains/WindowsMain.cpp
        Mains/LinuxMain.cpp
        Shaders/RuntimeShaderManager.h
        Shaders/RuntimeShaderManager.cpp
        Shaders/RuntimeShaderCode.cpp
        Shaders/RuntimeShaderCode.h
        Python/RuntimePythonRunner.cpp
        Python/RuntimePythonRunner.h)

target_link_libraries(PluRuntime PUBLIC Engine)

add_custom_command(TARGET PluRuntime
        PRE_BUILD
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Runtime
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(PluRuntime PROPERTIES UNITY_BUILD ON)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE argparse::argparse)
find_package(Jolt CONFIG REQUIRED)
target_link_libraries(PluRuntime PRIVATE Jolt::Jolt)
target_compile_definitions(PluRuntime PRIVATE JPH_DEBUG_RENDERER)

target_include_directories(PluRuntime PUBLIC ../ReflectionCache/Runtime)

set_target_properties(PluRuntime PROPERTIES
        # Szukaj .so w tym samym folderze co executable
        INSTALL_RPATH "$ORIGIN"
        # Użyj install RPATH już podczas budowania
        BUILD_WITH_INSTALL_RPATH TRUE
)

add_custom_command(TARGET PluRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
        $<TARGET_FILE_DIR:PluRuntime>    # To weźmie folder, gdzie powstaje PluEditor.exe
)