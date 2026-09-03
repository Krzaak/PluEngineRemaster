# L5 — GameObject, components, scenes, the physics world and the snapshot builder.
plu_add_engine_module(PluEffects
        DEPENDS PluCore PluAssetTypes)

target_link_libraries(PluEffects PUBLIC glm json glad Jolt::Jolt)
target_compile_definitions(PluEffects PUBLIC JPH_DEBUG_RENDERER)
