project(Engine)

# The engine is ten layered shared libraries. Each one lives in LibEngine/<Module>/{include,src}
# with its own include root, its own export macro and its own reflection cache directory, and may
# link only the modules below it — the layering is stated once, in each module's CMakeLists, and
# from here on the linker is what enforces it. PythonTools/LayeringCheck.py checks the same rule at
# the source level, which catches a violation before it becomes a link error.
#
# Listed bottom-up; add_subdirectory follows this order so a module's dependencies already exist.
set(PLU_MODULES
        PluCore
        PluPlatform PluAssetCore
        PluAssetTypes
        PluRender PluPhysics PluScripting
        PluAssetPipeline
        PluGameplay
        PluApp
        PluEffects
        CACHE INTERNAL "Engine modules, bottom-up. Consumers copy these next to their executable."
)

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

# ─────────────────────────────────────────────────────────────────────────────
#  Third-party
# ─────────────────────────────────────────────────────────────────────────────
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE "${PLU_THIRDPARTY_DIR}/spdlog/include")

add_library(json INTERFACE)
target_include_directories(json INTERFACE "${PLU_THIRDPARTY_DIR}/nlohmann")

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "${PLU_THIRDPARTY_DIR}/stb_image")

add_library(pybind11 INTERFACE)
target_include_directories(pybind11 INTERFACE "${PLU_THIRDPARTY_DIR}/pybind11/include")

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${PLU_THIRDPARTY_DIR}/glm")

find_package(imgui CONFIG REQUIRED)

add_library(ImGuizmo STATIC "${PLU_THIRDPARTY_DIR}/ImGuizmo/ImGuizmo.cpp")
target_include_directories(ImGuizmo PUBLIC "${PLU_THIRDPARTY_DIR}/ImGuizmo")
set_target_properties(ImGuizmo PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ImGuizmo PUBLIC imgui::imgui)

# glad is SHARED, and has to be: it is nothing but global function pointers, filled in once by
# gladLoadGL() from the platform module. As a static archive every library that links it would get
# its own copy of that table and only the platform module's would be initialised — the first GL
# call from anywhere else would dereference a null pointer.
if (WIN32)
    add_library(glad SHARED "${PLU_THIRDPARTY_DIR}/glad/glad.c" "${PLU_THIRDPARTY_DIR}/glad/glad_wgl.c")
else ()
    add_library(glad SHARED "${PLU_THIRDPARTY_DIR}/glad/glad.c")
endif ()
target_include_directories(glad PUBLIC "${PLU_THIRDPARTY_DIR}/glad")
set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (WIN32)
    # glad's own export macro; without it nothing is visible outside the DLL.
    target_compile_definitions(glad PRIVATE GLAD_GLAPI_EXPORT_BUILD)
    target_compile_definitions(glad PUBLIC GLAD_GLAPI_EXPORT)
endif ()

find_package(OpenGL REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development)
find_package(Jolt CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
if (NOT WIN32)
    find_package(SDL3 CONFIG REQUIRED)
endif ()

# ─────────────────────────────────────────────────────────────────────────────
#  Generated code
# ─────────────────────────────────────────────────────────────────────────────
# Bootstrap on the first configure so the per-module globs have files to collect. Afterwards the
# EngineReflection target below keeps them regenerated on every build.
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/ReflectionCache/PluCore")
    execute_process(
            COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif ()

# One run for the whole engine, before any module compiles. It was a PRE_BUILD command on the
# single Engine target; with ten of them it has to be a target they all depend on, or it would run
# ten times per build.
add_custom_target(EngineReflection
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp LibEngine
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Engine asset UUID index -> ReflectionCache/EngineAssets.h (Plu::EngineAssets::*). Built from the
# .pluasset files by EngineAssetsIndexer.py, a different tool from the reflection generator above
# and so in need of its own hook.
#
# A custom TARGET rather than a PRE_BUILD command: PRE_BUILD is only honoured by the Visual Studio
# generators, and everywhere else CMake degrades it to PRE_LINK — which runs AFTER the compilation
# that needs this header. DEPENDS on the asset files means it re-indexes exactly when a .pluasset
# is added, removed or edited, and does nothing on every other build.
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

# ─────────────────────────────────────────────────────────────────────────────
#  Module helper
# ─────────────────────────────────────────────────────────────────────────────
# Everything every engine module shares: sources, generated sources, include roots, its own export
# flag, the precompiled header and the RPATH. A module's own CMakeLists adds only what is specific
# to it — which modules it depends on, and which third-party libraries it uses.
function(plu_add_engine_module NAME)
    cmake_parse_arguments(PLU_MOD "" "" "DEPENDS" ${ARGN})

    file(GLOB_RECURSE MOD_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
    file(GLOB_RECURSE MOD_HEADERS CONFIGURE_DEPENDS "include/*.h" "include/*.inl" "src/*.h")
    file(GLOB_RECURSE MOD_GENERATED CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/ReflectionCache/${NAME}/*.cpp")

    add_library(${NAME} SHARED ${MOD_SOURCES} ${MOD_HEADERS} ${MOD_GENERATED})
    add_dependencies(${NAME} EngineReflection EngineAssetsIndex)

    # Only this module's flag: its own symbols export, everything it imports from the modules below
    # resolves to an import. That is the whole reason each module has a macro of its own.
    string(TOUPPER ${NAME} MOD_UPPER)
    target_compile_definitions(${NAME} PRIVATE PLU_BUILD_${MOD_UPPER})

    target_include_directories(${NAME}
            PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/ReflectionCache/${NAME}"
            "${CMAKE_SOURCE_DIR}/ReflectionCache"     # EngineAssets.h
            "${CMAKE_SOURCE_DIR}/ThirdParty"
            "${CMAKE_SOURCE_DIR}/PluSTL"
            PRIVATE
            # Private headers next to the sources (Platforms/<os>/*.h) are reached with a path
            # relative to the module's src root.
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(${NAME} PUBLIC ${PLU_MOD_DEPENDS})

    target_compile_definitions(${NAME} PUBLIC $<$<CONFIG:Debug>:PLU_DEBUG>)
    if (NOT PLU_DISTRIBUTION)
        # A distribution build leaves this undefined on purpose, so GetEngineResourcesDir() returns
        # the directory next to the executable instead of the source tree.
        target_compile_definitions(${NAME} PRIVATE PLU_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
    endif ()

    target_precompile_headers(${NAME} PRIVATE
            "${CMAKE_SOURCE_DIR}/PluSTL/PluSTL_FWD.h"
            "${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/PluEngine/Core.h"
            "${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/PluEngine/Log.h"
            "${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/PluEngine/Timer.h"
            "${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/stdpch.h"
    )
    target_compile_options(${NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-include ${CMAKE_SOURCE_DIR}/LibEngine/PluCore/include/stdpch.h>
            # The generated pybind11 bindings exceed the COFF per-object section limit on MSVC.
            $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    )

    # OFF for the duration of the split: a unity build merges translation units, so a header that
    # forgets an include still compiles as long as a neighbour in the same batch supplies it. Turn
    # back ON once things have settled.
    set_target_properties(${NAME} PROPERTIES
            UNITY_BUILD OFF
            # Every engine library sits next to the executable that loads it.
            INSTALL_RPATH "$ORIGIN"
            BUILD_RPATH "$ORIGIN"
    )
endfunction()

foreach (PLU_MODULE IN LISTS PLU_MODULES)
    add_subdirectory(${PLU_MODULE})
endforeach ()

# ─────────────────────────────────────────────────────────────────────────────
#  Compatibility target
# ─────────────────────────────────────────────────────────────────────────────
# Consumers still say `target_link_libraries(... Engine)` and get the whole engine. PluApp does not
# depend on PluAssetPipeline in the include graph — the asset loaders register themselves through
# reflection rather than being called by name — so listing every module here is also what keeps the
# pipeline in the link.
add_library(Engine INTERFACE)
target_link_libraries(Engine INTERFACE ${PLU_MODULES})
