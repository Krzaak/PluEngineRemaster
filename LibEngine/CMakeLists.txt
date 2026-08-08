project(Engine)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h" "src/*.h")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

file(GLOB_RECURSE IMGUI_SOURCES "${PLU_THIRDPARTY_DIR}/imgui/*.cpp")

# Bootstrap reflection sources on the first configure so the GLOB below has files to
# collect. On later builds the PRE_BUILD command keeps them regenerated, so we skip the
# configure-time run to avoid generating twice whenever CMake reconfigures.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ReflectionCache/LibEngine")
    execute_process(
            COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

file(GLOB_RECURSE SOURCES_REFLECTION CONFIGURE_DEPENDS "../ReflectionCache/LibEngine/*.cpp")

# Engine asset UUID index -> ReflectionCache/LibEngine/EngineAssets.h (Plu::EngineAssets::*).
# Built from the .pluasset files by EngineAssetsIndexer.py, which is a different tool from the
# reflection generator above and therefore needs its own hook.
#
# A custom TARGET rather than a PRE_BUILD command on Engine: PRE_BUILD is only honoured by the
# Visual Studio generators, and everywhere else CMake degrades it to PRE_LINK — which runs AFTER
# the compilation that needs this header. A target Engine depends on always runs first, on every
# generator. Editor and Runtime inherit the ordering through their dependency on Engine.
#
# DEPENDS on the asset files (not PRE_BUILD's unconditional run) means it re-indexes exactly when
# a .pluasset is added, removed or edited, and does nothing on every other build.
file(GLOB_RECURSE PLU_ENGINE_ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/EngineAssets/*.pluasset")

add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/ReflectionCache/LibEngine/EngineAssets.h"
        COMMENT "Indexing engine assets (EngineAssets.h)"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/EngineAssetsIndexer.py"
        DEPENDS ${PLU_ENGINE_ASSET_FILES} "${CMAKE_SOURCE_DIR}/PythonTools/EngineAssetsIndexer.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM
)

add_custom_target(EngineAssetsIndex DEPENDS "${CMAKE_SOURCE_DIR}/ReflectionCache/LibEngine/EngineAssets.h")

add_library(Engine SHARED ${SOURCES} ${HEADERS} ${SOURCES_REFLECTION}
        include/PluEngine/Objects/EngineObjectManager.inl
        ./include/stdpch.h
        ../ReflectionCache/LibEngine/EngineAssets.h
)

set_target_properties(Engine PROPERTIES UNITY_BUILD ON)

add_dependencies(Engine EngineAssetsIndex)

add_custom_command(TARGET Engine
        PRE_BUILD
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(Engine PUBLIC ../ReflectionCache/LibEngine)
target_sources(Engine PUBLIC ../ReflectionCache/PluEngineBindings.cpp)

target_compile_definitions(Engine PRIVATE PLU_BUILD_DLL)

#dasda

target_compile_definitions(Engine PUBLIC
        $<$<CONFIG:Debug>:PLU_DEBUG>
        #$<$<CONFIG:RelWithDebInfo>:PLU_DEBUG>
)

target_include_directories(Engine
        PUBLIC
        include/
        ../ThirdParty/
        ../PluSTL/
        ../ReflectionCache/
        PRIVATE
        src/
)

find_package(Jolt CONFIG REQUIRED)
target_link_libraries(Engine PRIVATE Jolt::Jolt)
target_compile_definitions(Engine PRIVATE JPH_DEBUG_RENDERER)

if(NOT WIN32)
    find_package(SDL3 CONFIG REQUIRED)
    target_link_libraries(Engine PRIVATE SDL3::SDL3)
endif()

find_package(imgui CONFIG REQUIRED)
target_link_libraries(Engine PUBLIC imgui::imgui)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(Engine PUBLIC assimp::assimp)

# ---------- spdlog ----------
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE
        "${PLU_THIRDPARTY_DIR}/spdlog/include"
)

add_library(json INTERFACE)
target_include_directories(json INTERFACE
        "${PLU_THIRDPARTY_DIR}/nlohmann"
)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE
        "${PLU_THIRDPARTY_DIR}/stb_image"
)

add_library(pybind11 INTERFACE)
target_include_directories(pybind11 INTERFACE
        "${PLU_THIRDPARTY_DIR}/pybind11/include"
)

find_package(Python3 COMPONENTS Interpreter Development)
target_link_libraries(Engine PUBLIC Python3::Python)

target_precompile_headers(Engine PUBLIC "${CMAKE_SOURCE_DIR}/PluSTL/PluSTL_FWD.h")
target_precompile_headers(Engine PUBLIC "./include/PluEngine/Core.h")
target_precompile_headers(Engine PUBLIC "./include/PluEngine/Log.h")
target_precompile_headers(Engine PUBLIC "./include/PluEngine/Timer.h")
target_precompile_headers(Engine PUBLIC "./include/stdpch.h")

target_compile_options(Engine PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-include ${CMAKE_SOURCE_DIR}/LibEngine/include/stdpch.h>
        # Generated pybind11 bindings (PluEngineBindings.cpp) exceed the COFF
        # per-object section limit (65279) on MSVC → need /bigobj.
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

# W buildzie dystrybucyjnym NIE definiujemy PLU_PROJECT_ROOT — dzięki temu
# GetEngineResourcesDir() zwraca katalog obok exe (EditorDist/), a nie drzewo źródeł.
if(NOT PLU_DISTRIBUTION)
    target_compile_definitions(Engine PRIVATE
            PLU_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    )
endif()


target_sources(Engine PUBLIC
        ${PLU_THIRDPARTY_DIR}/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(Engine PUBLIC
        ${PLU_THIRDPARTY_DIR}/ImGuizmo
)


#target_compile_definitions(imgui PUBLIC IMGUI_API=__declspec(dllexport))

# ---------- glm ----------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
        "${PLU_THIRDPARTY_DIR}/glm"
)

if (WIN32)
    add_library(glad STATIC
            "${PLU_THIRDPARTY_DIR}/glad/glad.c"
            "${PLU_THIRDPARTY_DIR}/glad/glad_wgl.c"
    )
else ()
    add_library(glad STATIC
            "${PLU_THIRDPARTY_DIR}/glad/glad.c"
    )
endif ()
target_include_directories(glad PUBLIC "${PLU_THIRDPARTY_DIR}/glad")

find_package(OpenGL REQUIRED)
target_link_libraries(Engine PRIVATE OpenGL::GL)
target_link_libraries(Engine PUBLIC glad)

if(WIN32)

else()
    set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# ---------- Engine ----------
target_link_libraries(Engine PUBLIC
        spdlog
        glm
        json
        pybind11
        stb_image
)
if (WIN32)
    target_link_libraries(Engine PRIVATE
            dwmapi
            ole32
            shell32
            uuid
    )
endif ()
