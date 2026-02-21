project(PluEditor)

execute_process(
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -p Editor
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
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -p Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

find_package(assimp CONFIG REQUIRED)

target_include_directories(PluEditor PUBLIC ../ReflectionCache/Editor)
target_include_directories(PluEditor PUBLIC ../Editor)

target_link_libraries(PluEditor PRIVATE Engine assimp::assimp)

#Fonts
target_compile_definitions(PluEditor PUBLIC PLU_FONTS_DIR="${PLU_THIRDPARTY_DIR}/UI/Fonts/")
target_compile_definitions(PluEditor PUBLIC PLU_PYTHON_TOOLS_DIR="${CMAKE_SOURCE_DIR}/PythonTools/")


if(WIN32)
    add_custom_command(TARGET PluEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
            $<TARGET_FILE_DIR:PluEditor>    # To weźmie folder, gdzie powstaje PluEditor.exe
    )
endif()

add_custom_command(TARGET PluEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/EngineAssets/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/EngineAssets)
