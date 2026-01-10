project(Editor)

execute_process(
        COMMAND python3 "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGenerator.py" Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION "../ReflectionCache/Editor/*.cpp")
file(GLOB_RECURSE HEADERS "./*.h")
file(GLOB_RECURSE SOURCES "./*.cpp")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

add_executable(Editor WIN32 ${SOURCES_REFLECTION} ${HEADERS} ${SOURCES})

target_include_directories(Editor PUBLIC ../ReflectionCache/Editor)
target_include_directories(Editor PUBLIC ../Editor)

target_link_libraries(Editor PRIVATE Engine)

#Fonts
target_compile_definitions(Editor PUBLIC PLU_FONTS_DIR="${PLU_THIRDPARTY_DIR}/UI/Fonts/")


if(WIN32)
    add_custom_command(TARGET Editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
            $<TARGET_FILE_DIR:Editor>    # To weźmie folder, gdzie powstaje Editor.exe
    )
endif()
