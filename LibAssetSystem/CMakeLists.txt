project(AssetSystem)

file(GLOB_RECURSE SOURCES_REFLECTION CONFIGURE_DEPENDS "../ReflectionCache/LibAssetSystem/*.cpp")

add_library(AssetSystem SHARED
        ${SOURCES_REFLECTION}
        include/AssetInfo.h
        include/AssetManager.h
        include/IAssetLoader.h
        src/AssetInfo.cpp
        src/AssetManager.cpp
        src/IAssetLoader.cpp
)

target_include_directories(AssetSystem PUBLIC
        ./include
)

target_include_directories(AssetSystem PRIVATE
        ../ReflectionCache/LibAssetSystem
        ../ReflectionCache/LibEngine
        ../LibEngine/include
        ../ThirdParty/imgui
        ../ThirdParty/nlohmann
        ../PluSTL
)

find_package(Python3 COMPONENTS Interpreter Development)
target_link_libraries(AssetSystem PUBLIC Python3::Python)

option(PLU_ASSET_EDITOR_BUILD "Build AssetSystem for Editor" OFF)

if(PLU_ASSET_EDITOR_BUILD)
    target_compile_definitions(AssetSystem PUBLIC PLU_EDITOR)
endif()