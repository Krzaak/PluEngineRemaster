project(Engine)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "src/*.h")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

file(GLOB_RECURSE IMGUI_SOURCES "${PLU_THIRDPARTY_DIR}/imgui/*.cpp")
file(GLOB_RECURSE IMGUIFILEDIALOG_SOURCES "${PLU_THIRDPARTY_DIR}/ImGuiFileDialog/*.cpp")

execute_process(
        COMMAND python3 "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGenerator.py" Engine
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION "../ReflectionCache/Engine/*.cpp")

add_library(Engine SHARED ${SOURCES} ${HEADERS} ${SOURCES_REFLECTION}
        include/PluEngine/Objects/EngineObjectManager.inl
        src/PluEngine/Renderer/FrameBuffer.cpp
        include/PluEngine/Reflection/ReflectionBase.h
        src/PluEngine/Reflection/ReflectionBase.cpp
        include/PluEngine/PluUUID.h
        src/PluEngine/PluUUID.cpp
        include/PluEngine/Managers/AssetsManager.h
        include/PluEngine/Managers/ScenesManager.h
        include/PluEngine/Managers/PythonManager.h
        include/PluEngine/Managers/ShadersManager.h
        src/Platforms/Linux/SdlWindow.cpp
        src/Platforms/Linux/SdlWindow.h
        include/PluEngine/PluPaths.h
        include/PluEngine/Managers/DiskManager.h
        src/PluEngine/Managers/DiskManager.cpp
)

target_include_directories(Engine PUBLIC ../ReflectionCache/Engine)

target_compile_definitions(Engine PRIVATE PLU_BUILD_DLL)

#dasda

target_compile_definitions(Engine PUBLIC
        $<$<CONFIG:Debug>:PLU_DEBUG>
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

add_custom_target(python_deps
        COMMAND ${Python_EXECUTABLE} -m pip install ${CMAKE_SOURCE_DIR}/Python/requirements.txt
)


# ---------- spdlog ----------
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE
        "${PLU_THIRDPARTY_DIR}/spdlog/include"
)

add_library(json INTERFACE)
target_include_directories(json INTERFACE
        "${PLU_THIRDPARTY_DIR}/nlohmann"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC "${PLU_THIRDPARTY_DIR}/imgui")
target_link_libraries(Engine PUBLIC imgui)

add_library(imguiFileDialog STATIC ${IMGUIFILEDIALOG_SOURCES})
target_include_directories(imguiFileDialog PUBLIC
        "${PLU_THIRDPARTY_DIR}/ImGuiFileDialog"
)
target_link_libraries(Engine PUBLIC imguiFileDialog)

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
target_link_libraries(Engine PRIVATE glad)

if(WIN32)

else()
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(Engine PRIVATE glfw)
    set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)

    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(Engine PRIVATE SDL2::SDL2)
endif()

# ---------- Engine ----------
target_link_libraries(Engine PUBLIC
        spdlog
        glm
        json
)
if (WIN32)
    target_link_libraries(Engine PRIVATE
            dwmapi
    )
endif ()
