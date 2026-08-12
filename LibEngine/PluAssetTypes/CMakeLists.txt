# L2 — the asset data types themselves, plus the node graph and animation data.
plu_add_engine_module(PluAssetTypes DEPENDS PluCore PluAssetCore)

# glad: StaticMesh/SkeletalMesh still carry their GL buffer names.
target_link_libraries(PluAssetTypes PUBLIC glm glad)
