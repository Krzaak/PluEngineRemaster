project(Engine)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "src/*.h")

set(PLU_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

file(GLOB_RECURSE IMGUI_SOURCES "${PLU_THIRDPARTY_DIR}/imgui/*.cpp")

execute_process(
        COMMAND python3 "${CMAKE_SOURCE_DIR}/PythonTools/ReflectionGenerator.py" Engine
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(GLOB_RECURSE SOURCES_REFLECTION "../ReflectionCache/Engine/*.cpp")

add_library(Engine SHARED ${SOURCES} ${HEADERS} ${SOURCES_REFLECTION}
        include/PluEngine/Objects/EngineObjectManager.inl
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

add_custom_target(PythonDeps
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
set_target_properties(imgui PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
target_include_directories(imgui PRIVATE "${CMAKE_SOURCE_DIR}/Engine/include")


target_sources(Engine PRIVATE
        ${PLU_THIRDPARTY_DIR}/ImGuiFileDialog/ImGuiFileDialog.cpp
)

target_include_directories(Engine PUBLIC
        ${PLU_THIRDPARTY_DIR}/ImGuiFileDialog
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
