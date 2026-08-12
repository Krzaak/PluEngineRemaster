project(Engine)

# The engine source is split into layered modules, each with its own include root. They are
# still compiled into a single Engine target for now; the layering is enforced later by giving
# each module only the include roots of the modules below it.
set(PLU_MODULES
        PluCore
        PluPlatform PluAssetCore
        PluAssetTypes
        PluRender PluPhysics PluScripting
        PluAssetPipeline
        PluGameplay
        PluApp
)

set(SOURCES "")
set(HEADERS "")
set(PLU_MODULE_INCLUDE_DIRS "")
set(PLU_MODULE_SOURCE_DIRS "")
foreach(PLU_MODULE IN LISTS PLU_MODULES)
    file(GLOB_RECURSE PLU_MODULE_SOURCES CONFIGURE_DEPENDS "${PLU_MODULE}/src/*.cpp")
    file(GLOB_RECURSE PLU_MODULE_HEADERS CONFIGURE_DEPENDS
            "${PLU_MODULE}/include/*.h" "${PLU_MODULE}/include/*.inl" "${PLU_MODULE}/src/*.h")
    list(APPEND SOURCES ${PLU_MODULE_SOURCES})
    list(APPEND HEADERS ${PLU_MODULE_HEADERS})
    list(APPEND PLU_MODULE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${PLU_MODULE}/include")
    list(APPEND PLU_MODULE_SOURCE_DIRS  "${CMAKE_CURRENT_SOURCE_DIR}/${PLU_MODULE}/src")
endforeach()

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

file(GLOB_RECURSE IMGUI_SOURCES "${PLU_THIRDPARTY_DIR}/imgui/*.cpp")

# Bootstrap reflection sources on the first configure so the GLOB below has files to
# collect. On later builds the PRE_BUILD command keeps them regenerated, so we skip the
# configure-time run to avoid generating twice whenever CMake reconfigures.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ReflectionCache/PluCore")
    execute_process(
            COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

# Each module is its own reflection project, so its generated sources land in their own cache
# directory and can be compiled into that module's own library once the split completes.
set(SOURCES_REFLECTION "")
foreach(PLU_MODULE IN LISTS PLU_MODULES)
    file(GLOB_RECURSE PLU_MODULE_REFLECTION CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/ReflectionCache/${PLU_MODULE}/*.cpp")
    list(APPEND SOURCES_REFLECTION ${PLU_MODULE_REFLECTION})
    list(APPEND PLU_MODULE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/ReflectionCache/${PLU_MODULE}")
endforeach()

# Engine asset UUID index -> ReflectionCache/EngineAssets.h (Plu::EngineAssets::*).
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
        OUTPUT "${CMAKE_SOURCE_DIR}/ReflectionCache/EngineAssets.h"
        COMMENT "Indexing engine assets (EngineAssets.h)"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/EngineAssetsIndexer.py"
        DEPENDS ${PLU_ENGINE_ASSET_FILES} "${CMAKE_SOURCE_DIR}/PythonTools/EngineAssetsIndexer.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM
)

add_custom_target(EngineAssetsIndex DEPENDS "${CMAKE_SOURCE_DIR}/ReflectionCache/EngineAssets.h")

add_library(Engine SHARED ${SOURCES} ${HEADERS} ${SOURCES_REFLECTION}
        PluCore/include/stdpch.h
        ../ReflectionCache/EngineAssets.h
)

# OFF for the duration of the module split. A unity build merges translation units, so a header
# that forgets an include still compiles as long as some neighbour in the same batch supplies it.
# Grouping the sources per module reshuffles those batches and turns such omissions into random
# breakage, so they get fixed for real instead. Turn back ON once the split has settled.
set_target_properties(Engine PROPERTIES UNITY_BUILD OFF)

add_dependencies(Engine EngineAssetsIndex)

add_custom_command(TARGET Engine
        PRE_BUILD
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_sources(Engine PUBLIC ../ReflectionCache/PluEngineBindings.cpp)

# One binary for now, so every module exports. When a module becomes its own library it keeps
# only its own flag here and the other nine resolve to import.
target_compile_definitions(Engine PRIVATE
        PLU_BUILD_DLL
        PLU_BUILD_PLUCORE
        PLU_BUILD_PLUPLATFORM
        PLU_BUILD_PLUASSETCORE
        PLU_BUILD_PLUASSETTYPES
        PLU_BUILD_PLURENDER
        PLU_BUILD_PLUPHYSICS
        PLU_BUILD_PLUSCRIPTING
        PLU_BUILD_PLUASSETPIPELINE
        PLU_BUILD_PLUGAMEPLAY
        PLU_BUILD_PLUAPP
)

#dasda

target_compile_definitions(Engine PUBLIC
        $<$<CONFIG:Debug>:PLU_DEBUG>
        #$<$<CONFIG:RelWithDebInfo>:PLU_DEBUG>
)

target_include_directories(Engine
        PUBLIC
        ${PLU_MODULE_INCLUDE_DIRS}
        ../ThirdParty/
        ../PluSTL/
        ../ReflectionCache/
        PRIVATE
        # Private headers next to the sources (e.g. Platforms/<os>/*.h) are still reached with a
        # path relative to a module's src root, exactly as they were relative to LibEngine/src.
        ${PLU_MODULE_SOURCE_DIRS}
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
target_precompile_headers(Engine PUBLIC "./PluCore/include/PluEngine/Core.h")
target_precompile_headers(Engine PUBLIC "./PluCore/include/PluEngine/Log.h")
target_precompile_headers(Engine PUBLIC "./PluCore/include/PluEngine/Timer.h")
target_precompile_headers(Engine PUBLIC "./PluCore/include/stdpch.h")

target_compile_options(Engine PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-include ${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/stdpch.h>
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
