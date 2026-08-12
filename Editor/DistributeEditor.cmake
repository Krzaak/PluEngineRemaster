# =====================================================================================
#  DistributeEditor.cmake — packages a self-contained standalone editor.
#
#  Invoked by the DistributeEditor target as `cmake -P` with parameters:
#    DIST_DIR    - destination directory (EditorDist/)
#    SOURCE_DIR  - repo root
#    EDITOR_EXE  - full path to the built PluEditor(.exe)
#    ENGINE_LIBS - ;-separated paths to every engine module library (+ glad)
#    PYARMOR_EXE - pyarmor from the build venv
# =====================================================================================

foreach(_var DIST_DIR SOURCE_DIR EDITOR_EXE ENGINE_LIBS PYARMOR_EXE)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "DistributeEditor.cmake: missing required variable ${_var}")
    endif()
endforeach()

message(STATUS "[dist] Cleaning ${DIST_DIR}")
file(REMOVE_RECURSE "${DIST_DIR}")
file(MAKE_DIRECTORY "${DIST_DIR}")

# --- Binaries --------------------------------------------------------------------------
message(STATUS "[dist] Copying binaries")
file(COPY "${EDITOR_EXE}" DESTINATION "${DIST_DIR}")
# The engine is ten shared libraries now, not one, and the executable finds them through an
# $ORIGIN RPATH — so all of them have to land next to it.
foreach(_lib IN LISTS ENGINE_LIBS)
    file(COPY "${_lib}" DESTINATION "${DIST_DIR}")
endforeach()

# Windows: copy over every DLL sitting next to PluEditor.exe. vcpkg (applocal deployment)
# already resolved the full, transitive runtime dependencies there based on the actual PE
# imports — including ones $<TARGET_RUNTIME_DLLS:PluEditor> can't see because they're linked
# PRIVATE by an intermediate dependency (e.g. assimp → kubazip/minizip/poly2tri/pugixml/zlib).
if(WIN32)
    get_filename_component(_editor_exe_dir "${EDITOR_EXE}" DIRECTORY)
    message(STATUS "[dist] Copying dependency DLLs from ${_editor_exe_dir}")
    file(GLOB _dep_dlls "${_editor_exe_dir}/*.dll")
    file(COPY ${_dep_dlls} DESTINATION "${DIST_DIR}")
endif()

# --- Engine resources --------------------------------------------------------------------
message(STATUS "[dist] Copying EngineAssets")
file(COPY "${SOURCE_DIR}/EngineAssets" DESTINATION "${DIST_DIR}")

# Fonts + icons (GetEngineResourcesDir()/ThirdParty/UI/Fonts/...)
message(STATUS "[dist] Copying ThirdParty/UI")
file(MAKE_DIRECTORY "${DIST_DIR}/ThirdParty")
file(COPY "${SOURCE_DIR}/ThirdParty/UI" DESTINATION "${DIST_DIR}/ThirdParty")

# Type stub for projects' Python environment
if(EXISTS "${SOURCE_DIR}/ReflectionCache/PluEngine.pyi")
    file(COPY "${SOURCE_DIR}/ReflectionCache/PluEngine.pyi" DESTINATION "${DIST_DIR}")
endif()

# --- Obfuscate ShaderCodeParser.py -------------------------------------------------------
# The only Python tool run at editor runtime (EditorShaderManager
# → RunScript(GetEngineResourcesDir()/PythonTools/ShaderCodeParser.py)). PyArmor gen
# creates a pyarmor_runtime_* package next to the obfuscated script, which must be
# copied alongside it.
set(_pytools_dir "${DIST_DIR}/PythonTools")
file(MAKE_DIRECTORY "${_pytools_dir}")

message(STATUS "[dist] Obfuscating ShaderCodeParser.py (PyArmor)")
execute_process(
        COMMAND "${PYARMOR_EXE}" gen --output "${_pytools_dir}"
                "${SOURCE_DIR}/PythonTools/ShaderCodeParser.py"
        RESULT_VARIABLE _pyarmor_res
        OUTPUT_VARIABLE _pyarmor_out
        ERROR_VARIABLE  _pyarmor_err
)
if(NOT _pyarmor_res EQUAL 0)
    message(FATAL_ERROR "[dist] PyArmor failed (${_pyarmor_res}):\n${_pyarmor_out}\n${_pyarmor_err}")
endif()

if(NOT EXISTS "${_pytools_dir}/ShaderCodeParser.py")
    message(FATAL_ERROR "[dist] PyArmor did not generate ShaderCodeParser.py in ${_pytools_dir}")
endif()

message(STATUS "[dist] Done: ${DIST_DIR}")
