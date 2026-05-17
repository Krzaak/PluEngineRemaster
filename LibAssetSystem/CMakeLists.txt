project(AssetSystem)

add_library(AssetSystem SHARED
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
)