# L6 — Application, and the one place that links every module: the generated pybind11 bindings
# name types from all of them.
plu_add_engine_module(PluApp
        DEPENDS PluCore PluAssetCore PluPlatform PluAssetTypes PluScripting
                PluRender PluPhysics PluAssetPipeline PluGameplay)

target_sources(PluApp PRIVATE "${CMAKE_SOURCE_DIR}/ReflectionCache/PluEngineBindings.cpp")
target_link_libraries(PluApp PUBLIC argparse::argparse)
