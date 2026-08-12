# L5 — GameObject, components, scenes, the physics world and the snapshot builder.
plu_add_engine_module(PluGameplay
        DEPENDS PluCore PluAssetCore PluPlatform PluAssetTypes PluRender PluPhysics)

target_link_libraries(PluGameplay PUBLIC glm json glad Jolt::Jolt)
target_compile_definitions(PluGameplay PUBLIC JPH_DEBUG_RENDERER)
