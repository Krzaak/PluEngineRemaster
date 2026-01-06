project(Editor)

execute_process(
        COMMAND python3 "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGenerator.py" Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION "../ReflectionCache/Editor/*.cpp")

add_executable(Editor WIN32 ${SOURCES_REFLECTION}
        Mains/WindowsMain.cpp
        Mains/LinuxMain.cpp
        EditorApp.h
        EditorApp.cpp
        Panels/EditorPanelManager.cpp
        Panels/EditorPanelManager.h
        Panels/EditorPanel.cpp
        Panels/EditorPanel.h
        DefinedPanels/EngineStatsPanel.cpp
        DefinedPanels/EngineStatsPanel.h
        Managers/Assets/EditorAssetManager.cpp
        Managers/Assets/EditorAssetManager.h
        ../PluSTL/HashMap/HashMap.h
        DefinedPanels/Style/EditorStylePanel.cpp
        DefinedPanels/Style/EditorStylePanel.h
        Managers/Project/EditorProjectManager.cpp
        Managers/Project/EditorProjectManager.h)

target_include_directories(Editor PUBLIC ../ReflectionCache/Editor)
target_include_directories(Editor PUBLIC ../Editor)

target_link_libraries(Editor PRIVATE Engine)

if(WIN32)
    add_custom_command(TARGET Editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>        # To automatycznie weźmie libEngine.dll lub Engine.dll
            $<TARGET_FILE_DIR:Editor>    # To weźmie folder, gdzie powstaje Editor.exe
    )
endif()
