project(PluEditor)

execute_process(
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION "../ReflectionCache/Editor/*.cpp")
file(GLOB_RECURSE HEADERS "./*.h")
file(GLOB_RECURSE SOURCES "./*.cpp")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

add_executable(PluEditor ${SOURCES_REFLECTION} ${HEADERS} ${SOURCES}
        ../PluSTL/Path/Path.h
        Managers/Assets/EditorAssetObject.h)

add_custom_command(TARGET PluEditor
        PRE_BUILD
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

find_package(assimp CONFIG REQUIRED)

target_include_directories(PluEditor PUBLIC ../ReflectionCache/Editor)
target_include_directories(PluEditor PUBLIC ../Editor)

target_link_libraries(PluEditor PRIVATE Engine assimp::assimp)
find_package(Jolt CONFIG REQUIRED)
target_link_libraries(PluEditor PRIVATE Jolt::Jolt)
target_compile_definitions(PluEditor PRIVATE JPH_DEBUG_RENDERER)

target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/PluSTL/PluSTL_FWD.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/Engine/include/PluEngine/Core.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/Engine/include/PluEngine/Log.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/Engine/include/PluEngine/Timer.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/Engine/include/stdpch.h")

target_compile_options(PluEditor PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-include ${CMAKE_SOURCE_DIR}/Engine/include/stdpch.h>
)


if(WIN32)
    add_custom_command(TARGET PluEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
            $<TARGET_FILE_DIR:PluEditor>    # To weźmie folder, gdzie powstaje PluEditor.exe
    )
endif()

add_custom_command(TARGET PluEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/ReflectionCache/PluEngine.pyi
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Exe
install(TARGETS PluEditor
        RUNTIME DESTINATION bin
)

# Engine.dll
install(FILES $<TARGET_FILE:Engine>
        DESTINATION bin
)

# Foldery z assetami
install(DIRECTORY ${CMAKE_SOURCE_DIR}/EngineAssets/
        DESTINATION bin/EngineAssets
)

# Plik .pyi
install(FILES ${CMAKE_SOURCE_DIR}/ReflectionCache/PluEngine.pyi
        DESTINATION bin
)

# Runtime DLL-ki (już masz, zostawiam)
install(FILES $<TARGET_RUNTIME_DLLS:PluEditor>
        DESTINATION bin
)

set(CPACK_PACKAGE_NAME "PluEditor")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Plutex")
set(CPACK_GENERATOR "ZIP")
set(CPACK_STRIP_FILES TRUE)          # usuwa debug symbols → mniejszy plik
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)  # brak zbędnego folderu root w ZIP

include(CPack)
