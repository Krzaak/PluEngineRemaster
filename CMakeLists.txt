cmake_minimum_required(VERSION 4.0.2)
# --- 1. POSZUKIWANIE VCPKG ---
# Najpierw sprawdźmy, czy użytkownik ma zmienną środowiskową VCPKG_ROOT
if(DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_PATH "$ENV{VCPKG_ROOT}")
    message(STATUS "Znaleziono vcpkg w systemie (VCPKG_ROOT): ${VCPKG_PATH}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/.vcpkg-root")
    set(VCPKG_PATH "${CMAKE_SOURCE_DIR}/vcpkg")
    message(STATUS "Znaleziono lokalną kopię vcpkg: ${VCPKG_PATH}")
else()
    # Jeśli nigdzie nie ma, pobieramy do folderu projektu
    message(STATUS "Nie znaleziono vcpkg. Pobieranie do ${CMAKE_SOURCE_DIR}/vcpkg...")
    set(VCPKG_PATH "${CMAKE_SOURCE_DIR}/vcpkg")
    execute_process(COMMAND git clone https://github.com/microsoft/vcpkg.git ${VCPKG_PATH})
    message(STATUS "Pobrano vcpkg!")

    if(WIN32)
        execute_process(COMMAND ${VCPKG_PATH}/bootstrap-vcpkg.bat)
    else()
        execute_process(COMMAND ${VCPKG_PATH}/bootstrap-vcpkg.sh)
    endif()
endif()

if(EXISTS "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
    include("${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake") # Wymuszenie załadowania
endif()

set(VCPKG_MANIFEST_MODE ON CACHE BOOL "")
project(PluEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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
    add_compile_options(/permissive- /W4 /utf-8)
    # Wyłącz optymalizacje dla wersji Debug
    add_compile_options($<$<CONFIG:Debug>:/Od>)
    # Linker musi stworzyć plik .pdb
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

#Time
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
add_compile_definitions(PLU_BUILD_TIME="${BUILD_TIME}")



add_subdirectory(Editor)
add_subdirectory(Engine)
add_subdirectory(PluSTL)
add_subdirectory(Runtime)
