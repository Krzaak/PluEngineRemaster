# =====================================================================================
#  DistributeEditor.cmake — skrypt pakujący samowystarczalny edytor.
#
#  Uruchamiany przez target DistributeEditor jako `cmake -P` z parametrami:
#    DIST_DIR    - katalog docelowy (EditorDist/)
#    SOURCE_DIR  - root repo
#    EDITOR_EXE  - pełna ścieżka do zbudowanego PluEditor(.exe)
#    ENGINE_LIB  - pełna ścieżka do libEngine.so / Engine.dll
#    PYARMOR_EXE - pyarmor z build-venva
# =====================================================================================

foreach(_var DIST_DIR SOURCE_DIR EDITOR_EXE ENGINE_LIB PYARMOR_EXE)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "DistributeEditor.cmake: brak wymaganej zmiennej ${_var}")
    endif()
endforeach()

message(STATUS "[dist] Czyszczenie ${DIST_DIR}")
file(REMOVE_RECURSE "${DIST_DIR}")
file(MAKE_DIRECTORY "${DIST_DIR}")

# --- Binaria -------------------------------------------------------------------------
message(STATUS "[dist] Kopiowanie binariów")
file(COPY "${EDITOR_EXE}" DESTINATION "${DIST_DIR}")
file(COPY "${ENGINE_LIB}" DESTINATION "${DIST_DIR}")

# Windows: dokopiuj wszystkie DLL-e leżące obok PluEditor.exe. vcpkg (applocal deployment)
# już rozwiązał tam pełne, tranzytywne zależności runtime na podstawie realnych importów PE —
# w tym te, których $<TARGET_RUNTIME_DLLS:PluEditor> nie widzi, bo są linkowane PRIVATE przez
# pośrednie zależności (np. assimp → kubazip/minizip/poly2tri/pugixml/zlib).
if(WIN32)
    get_filename_component(_editor_exe_dir "${EDITOR_EXE}" DIRECTORY)
    message(STATUS "[dist] Kopiowanie zależnych DLL-i z ${_editor_exe_dir}")
    file(GLOB _dep_dlls "${_editor_exe_dir}/*.dll")
    file(COPY ${_dep_dlls} DESTINATION "${DIST_DIR}")
endif()

# --- Zasoby silnika ------------------------------------------------------------------
message(STATUS "[dist] Kopiowanie EngineAssets")
file(COPY "${SOURCE_DIR}/EngineAssets" DESTINATION "${DIST_DIR}")

# Fonty + ikony (GetEngineResourcesDir()/ThirdParty/UI/Fonts/...)
message(STATUS "[dist] Kopiowanie ThirdParty/UI")
file(MAKE_DIRECTORY "${DIST_DIR}/ThirdParty")
file(COPY "${SOURCE_DIR}/ThirdParty/UI" DESTINATION "${DIST_DIR}/ThirdParty")

# Stub typów dla środowiska pythonowego projektów
if(EXISTS "${SOURCE_DIR}/ReflectionCache/PluEngine.pyi")
    file(COPY "${SOURCE_DIR}/ReflectionCache/PluEngine.pyi" DESTINATION "${DIST_DIR}")
endif()

# --- Obfuskacja ShaderCodeParser.py --------------------------------------------------
# To jedyne narzędzie pythonowe uruchamiane w runtime edytora (EditorShaderManager
# → RunScript(GetEngineResourcesDir()/PythonTools/ShaderCodeParser.py)). PyArmor gen
# tworzy obok obfuskowanego skryptu pakiet pyarmor_runtime_*, który musi zostać
# skopiowany razem z nim.
set(_pytools_dir "${DIST_DIR}/PythonTools")
file(MAKE_DIRECTORY "${_pytools_dir}")

message(STATUS "[dist] Obfuskacja ShaderCodeParser.py (PyArmor)")
execute_process(
        COMMAND "${PYARMOR_EXE}" gen --output "${_pytools_dir}"
                "${SOURCE_DIR}/PythonTools/ShaderCodeParser.py"
        RESULT_VARIABLE _pyarmor_res
        OUTPUT_VARIABLE _pyarmor_out
        ERROR_VARIABLE  _pyarmor_err
)
if(NOT _pyarmor_res EQUAL 0)
    message(FATAL_ERROR "[dist] PyArmor nie powiódł się (${_pyarmor_res}):\n${_pyarmor_out}\n${_pyarmor_err}")
endif()

if(NOT EXISTS "${_pytools_dir}/ShaderCodeParser.py")
    message(FATAL_ERROR "[dist] PyArmor nie wygenerował ShaderCodeParser.py w ${_pytools_dir}")
endif()

message(STATUS "[dist] Gotowe: ${DIST_DIR}")
