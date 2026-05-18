cmake_minimum_required(VERSION 4.0.2)
# Minimalna wymagana wersja vcpkg (data bazując na commicie/roku)
set(VCPKG_MIN_VERSION "2024-01-01")

# --- Sprawdzenie wersji systemowego vcpkg ---
function(check_vcpkg_version vcpkg_exe out_ok)
    execute_process(
            COMMAND ${vcpkg_exe} version
            OUTPUT_VARIABLE _vcpkg_ver_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "[0-9]{4}-[0-9]{2}-[0-9]{2}" _vcpkg_date "${_vcpkg_ver_output}")
    if(_vcpkg_date STREQUAL "")
        set(${out_ok} FALSE PARENT_SCOPE)
        return()
    endif()
    if(_vcpkg_date STRLESS "${VCPKG_MIN_VERSION}")
        message(STATUS "System vcpkg (${_vcpkg_date}) is too old (required >= ${VCPKG_MIN_VERSION})")
        set(${out_ok} FALSE PARENT_SCOPE)
    else()
        message(STATUS "System vcpkg (${_vcpkg_date}) meets requirements (>= ${VCPKG_MIN_VERSION})")
        set(${out_ok} TRUE PARENT_SCOPE)
    endif()
endfunction()

# --- Ustal ścieżkę do vcpkg ---
set(VCPKG_PATH "")

if(DEFINED ENV{VCPKG_ROOT})
    set(_sys_vcpkg_exe "$ENV{VCPKG_ROOT}/vcpkg")
    check_vcpkg_version("${_sys_vcpkg_exe}" _sys_vcpkg_ok)
    if(_sys_vcpkg_ok)
        set(VCPKG_PATH "$ENV{VCPKG_ROOT}")
        message(STATUS "Using system vcpkg (VCPKG_ROOT): ${VCPKG_PATH}")
    endif()
endif()

if(VCPKG_PATH STREQUAL "" AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/.vcpkg-root")
    set(_local_vcpkg_exe "${CMAKE_SOURCE_DIR}/vcpkg/vcpkg")
    check_vcpkg_version("${_local_vcpkg_exe}" _local_vcpkg_ok)
    if(_local_vcpkg_ok)
        set(VCPKG_PATH "${CMAKE_SOURCE_DIR}/vcpkg")
        message(STATUS "Using local vcpkg copy: ${VCPKG_PATH}")
    else()
        message(STATUS "Local vcpkg copy is too old — updating...")
        execute_process(
                COMMAND git -C "${CMAKE_SOURCE_DIR}/vcpkg" pull
                RESULT_VARIABLE _git_pull_result
        )
        if(_git_pull_result EQUAL 0)
            set(VCPKG_PATH "${CMAKE_SOURCE_DIR}/vcpkg")
            message(STATUS "Local vcpkg updated successfully.")
        else()
            message(WARNING "Failed to update local vcpkg.")
        endif()
    endif()
endif()

if(VCPKG_PATH STREQUAL "")
    message(STATUS "No suitable vcpkg found. Cloning into ${CMAKE_SOURCE_DIR}/vcpkg...")
    set(VCPKG_PATH "${CMAKE_SOURCE_DIR}/vcpkg")
    execute_process(COMMAND git clone https://github.com/microsoft/vcpkg.git ${VCPKG_PATH})
    message(STATUS "vcpkg cloned successfully!")

    if(WIN32)
        execute_process(COMMAND ${VCPKG_PATH}/bootstrap-vcpkg.bat)
    else()
        execute_process(COMMAND ${VCPKG_PATH}/bootstrap-vcpkg.sh -disableMetrics)
    endif()
endif()

# --- Load toolchain ---
if(EXISTS "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_OVERLAY_PORTS "${CMAKE_SOURCE_DIR}/vcpkg-overlays" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
    include("${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake")
endif()

set(VCPKG_MANIFEST_MODE ON CACHE BOOL "")
project(PluEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CONFIGURATION_TYPES
        Debug
        Release
        Distribution
        CACHE STRING "" FORCE
)


option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")

# --- 2. KONFIGURACJA PYTHON VENV ---
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(WIN32)
    set(VENV_NAME "venv-windows")
    set(VENV_BIN "Scripts")
    set(PYTHON_EXE "python.exe")
else()
    set(VENV_NAME "venv-linux")
    set(VENV_BIN "bin")
    set(PYTHON_EXE "python")
endif()

set(VENV_PATH "${CMAKE_SOURCE_DIR}/Python/${VENV_NAME}")
set(REQ_FILE "${CMAKE_SOURCE_DIR}/Python/requirements.txt")
set(STAMP_FILE "${VENV_PATH}/requirements.timestamp")
set(VENV_EXECUTABLE "${VENV_PATH}/${VENV_BIN}/${PYTHON_EXE}")

# Tworzenie venv
if(NOT EXISTS "${VENV_PATH}")
    execute_process(COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_PATH})
endif()

# Inteligentna instalacja requirements
if(EXISTS "${REQ_FILE}")
    if("${REQ_FILE}" IS_NEWER_THAN "${STAMP_FILE}")
        message(STATUS "Aktualizacja pakietów Python...")
        execute_process(COMMAND "${VENV_PATH}/${VENV_BIN}/pip" install -r "${REQ_FILE}")
        file(WRITE "${STAMP_FILE}" "")
    endif()
endif()


if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_WARNINGS
    )
endif()

if(MSVC)
    # Wymusza standardy i rygorystyczne sprawdzanie kodu (podobnie jak Clang)
    message("PluMSVC")
    add_compile_options(/permissive- /W4 /utf-8 /MP)
    # Wyłącz optymalizacje dla wersji Debug
    add_compile_options($<$<CONFIG:Debug>:/Od>)
    # Linker musi stworzyć plik .pdb
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()


add_subdirectory(LibEngine)
add_subdirectory(Editor)
add_subdirectory(PluSTL)
add_subdirectory(Runtime)
