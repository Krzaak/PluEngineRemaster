# L3 — the Jolt-facing layer: shapes, bodies, layers, collision rules, materials.
plu_add_engine_module(PluPhysics DEPENDS PluCore PluAssetTypes)

target_link_libraries(PluPhysics PUBLIC glm Jolt::Jolt)
target_compile_definitions(PluPhysics PUBLIC JPH_DEBUG_RENDERER)
