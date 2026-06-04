project(PluEditor)

execute_process(
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION CONFIGURE_DEPENDS "../ReflectionCache/Editor/*.cpp")
#file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "./*.h")
#file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "./*.cpp")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

add_executable(PluEditor WIN32 ${SOURCES_REFLECTION}
        ./DefinedPanels/EditorSettingsPanel.cpp
        ./DefinedPanels/EngineClassTreePanel.cpp
        ./DefinedPanels/EngineStatsPanel.cpp
        ./DefinedPanels/InputViewerPanel.cpp
        ./DefinedPanels/Project/AssetBrowserPanel/AssetBrowserPanel.cpp
        ./DefinedPanels/ProjectLauncherPanel.cpp
        ./DefinedPanels/Style/EditorStylePanel.cpp
        ./DefinedViewports/Material/MaterialDetailsPanel.cpp
        ./DefinedViewports/Material/MaterialViewport.cpp
        ./DefinedViewports/Scene/SceneObjectDetailsPanel.cpp
        ./DefinedViewports/Scene/SceneStructurePanel.cpp
        ./DefinedViewports/Scene/SceneViewport.cpp
        ./DefinedViewports/Scene/SceneViewportPanel.cpp
        ./DefinedViewports/Scene/SceneWorldSettings.cpp
        ./DefinedViewports/StaticMesh/StaticMeshDetailsPanel.cpp
        ./DefinedViewports/StaticMesh/StaticMeshViewport.cpp
        ./DefinedViewports/StaticMesh/StaticMeshViewportPanel.cpp
        ./DefinedViewports/Texture/TextureDisplayPanel.cpp
        ./DefinedViewports/Texture/TextureViewport.cpp
        ./EditorApp.cpp
        ./EditorSettings/EditorSettingsManager.cpp
        ./EditorViewports/EditorViewportManager.cpp
        ./EditorViewports/IEditorPanel.cpp
        ./EditorViewports/IEditorViewport.cpp
        ./EditorWindows/EditorWindowsManager.cpp
        ./Mains/LinuxMain.cpp
        ./Mains/WindowsMain.cpp
        ./Managers/Assets/EditorAssetManager.cpp
        ./Managers/Project/EditorProjectManager.cpp
        ./Managers/Python/EditorPythonManager.cpp
        ./Managers/Scene/EditorCamera.cpp
        ./Managers/Shaders/EditorShaderCode.cpp
        ./Managers/Shaders/EditorShaderManager.cpp
        ./Panels/EditorPanel.cpp
        ./Panels/EditorPanelManager.cpp

        ./DefinedPanels/EditorSettingsPanel.h
        ./DefinedPanels/EngineClassTreePanel.h
        ./DefinedPanels/EngineStatsPanel.h
        ./DefinedPanels/InputViewerPanel.h
        ./DefinedPanels/Project/AssetBrowserPanel/AssetBrowserPanel.h
        ./DefinedPanels/ProjectLauncherPanel.h
        ./DefinedPanels/Style/EditorStylePanel.h
        ./DefinedViewports/Material/MaterialDetailsPanel.h
        ./DefinedViewports/Material/MaterialViewport.h
        ./DefinedViewports/Scene/SceneObjectDetailsPanel.h
        ./DefinedViewports/Scene/SceneStructurePanel.h
        ./DefinedViewports/Scene/SceneViewport.h
        ./DefinedViewports/Scene/SceneViewportPanel.h
        ./DefinedViewports/Scene/SceneWorldSettings.h
        ./DefinedViewports/StaticMesh/StaticMeshDetailsPanel.h
        ./DefinedViewports/StaticMesh/StaticMeshViewport.h
        ./DefinedViewports/StaticMesh/StaticMeshViewportPanel.h
        ./DefinedViewports/Texture/TextureDisplayPanel.h
        ./DefinedViewports/Texture/TextureViewport.h
        ./EditorApp.h
        ./EditorAppContext.h
        ./EditorInterface.h
        ./EditorSettings/EditorSettings.h
        ./EditorSettings/EditorSettingsManager.h
        ./EditorViewports/EditorViewportManager.h
        ./EditorViewports/IEditorPanel.h
        ./EditorViewports/IEditorViewport.h
        ./EditorWindows/EditorWindowsManager.h
        ./Managers/Assets/EditorAssetImporter.h
        ./Managers/Assets/EditorAssetManager.h
        ./Managers/Project/EditorProjectManager.h
        ./Managers/Project/PythonBuildEnvironment.h
        ./Managers/Python/EditorPythonManager.h
        ./Managers/Scene/EditorCamera.h
        ./Managers/Shaders/EditorShaderCode.h
        ./Managers/Shaders/EditorShaderManager.h
        ./Panels/EditorPanel.h
        ./Panels/EditorPanelManager.h
        ./Utils/RGBTransformDragger.h
        DefinedPanels/TextureViewerPanel.h
        DefinedPanels/TextureViewerPanel.cpp
        Managers/Assets/EditorAssetCreator.cpp
        Managers/Assets/EditorAssetCreator.h
        Managers/Assets/EditorAssetImporter.cpp
        Managers/Assets/EditorAssetImporter.h
        DefinedPanels/Project/ProjectSettings/ProjectSettingsPanel.cpp
        DefinedPanels/Project/ProjectSettings/ProjectSettingsPanel.h
)

target_compile_definitions(PluEditor PUBLIC PLU_EDITOR)

add_custom_command(TARGET PluEditor
        PRE_BUILD
        COMMENT "Generating Reflection Data"
        COMMAND ${VENV_EXECUTABLE} "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGeneratorRegex.py" -bp Editor
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(PluEditor PROPERTIES UNITY_BUILD ON)

find_package(argparse CONFIG REQUIRED)

target_include_directories(PluEditor PUBLIC ../ReflectionCache/Editor)
target_include_directories(PluEditor PUBLIC ../Editor)

target_link_libraries(PluEditor PRIVATE Engine argparse::argparse)
find_package(Jolt CONFIG REQUIRED)
target_link_libraries(PluEditor PRIVATE Jolt::Jolt)
target_compile_definitions(PluEditor PRIVATE JPH_DEBUG_RENDERER)

target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/PluSTL/PluSTL_FWD.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/LibEngine/include/PluEngine/Core.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/LibEngine/include/PluEngine/Log.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/LibEngine/include/PluEngine/Timer.h")
target_precompile_headers(PluEditor PUBLIC "${CMAKE_SOURCE_DIR}/LibEngine/include/stdpch.h")

target_compile_options(PluEditor PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-include ${CMAKE_SOURCE_DIR}/LibEngine/include/stdpch.h>
)

find_package(efsw CONFIG REQUIRED)
target_link_libraries(PluEditor PRIVATE efsw::efsw)


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
