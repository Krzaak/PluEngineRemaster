# L1 — window, OS event pumping, input backends.
plu_add_engine_module(PluPlatform DEPENDS PluCore)

target_link_libraries(PluPlatform PUBLIC glad imgui::imgui)
if (NOT WIN32)
    target_link_libraries(PluPlatform PRIVATE SDL3::SDL3)
else ()
    target_link_libraries(PluPlatform PRIVATE dwmapi ole32 shell32 uuid)
endif ()
