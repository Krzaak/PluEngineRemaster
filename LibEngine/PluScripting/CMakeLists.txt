# L3 — pybind11 glue. Optional in principle: nothing below it names Python.
plu_add_engine_module(PluScripting DEPENDS PluCore)

target_link_libraries(PluScripting PUBLIC glm pybind11 Python3::Python)
