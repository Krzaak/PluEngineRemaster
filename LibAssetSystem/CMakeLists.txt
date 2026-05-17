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
        ../LibEngine/include
        ../ReflectionCache/LibAssetSystem
)