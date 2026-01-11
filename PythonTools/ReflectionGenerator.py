import clang.cindex
import os
import json
import shlex
import subprocess
import re
import json
from datetime import datetime
from pathlib import Path
import sys
import EngineUtils

# --- KONFIGURACJA ŚCIEŻEK ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
GURU_PROJECT_ROOT = PROJECT_ROOT
BUILD_DIR = os.path.join(PROJECT_ROOT, "cmake-build-debug")
DATABASE_FILE = os.path.join(BUILD_DIR, "compile_commands.json")
OUTPUT_FILE = os.path.join(PROJECT_ROOT, "ReflectionCache")

if sys.platform.__contains__("win32"):
    print("On Windows!")
    BUILD_DIR = os.path.join(PROJECT_ROOT, "cmake-build-debug")
    DATABASE_FILE = os.path.join(BUILD_DIR, "compile_commands.json")
    clang.cindex.Config.set_library_path(r"D:\ProgramsPlo\LLVM\bin")

try:
    SubprojectToReprocess = sys.argv[1]
    if SubprojectToReprocess != "ALL":
        PROJECT_ROOT = os.path.join(PROJECT_ROOT, SubprojectToReprocess)
    else:
        print("ALL")
except:
    SubprojectToReprocess = ""
    print("No project selected!")
    exit()

def get_clang_resource_dir():
    try:
        return subprocess.check_output(["clang", "-print-resource-dir"]).decode().strip()
    except:
        return ""

def filter_args(args):
    # Dodajemy driver-mode, aby uniknąć błędów "linker input unused"
    filtered = ["--driver-mode=g++"]

    # Lista folderów, które ZAWSZE powinny być w Include
    extra_includes = [
        os.path.join(GURU_PROJECT_ROOT, "Engine", "include"),
        os.path.join(GURU_PROJECT_ROOT, "Editor"),
        os.path.join(GURU_PROJECT_ROOT, "ThirdParty", "glad")
    ]

    for inc in extra_includes:
        filtered.append(f"-I{inc}")

    # Twoja dotychczasowa logika filtrowania...
    forbidden = ('-fmodules-ts', '-fmodule-mapper', '-fdeps-format', '-fpreprocessed', '-MT', '-MF', '-MD', '-MP')
    skip_next = False

    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg in ('-MF', '-MT', '-o', '/Fo', '/Fd', '/c'):
            skip_next = True
            continue

        # Ignoruj flagi MSVC i stare standardy
        if arg.startswith(('/std:', '-std:', '/Zi', '/RTC', '/EH', '/nologo')):
            continue

        if not arg.startswith(forbidden) and not arg.startswith('/'):
            filtered.append(arg)

    # Wymuszenie nowoczesnego C++
    filtered.extend(["-x", "c++", "-std=c++20", "-Wno-everything", "-ferror-limit=0", "-DPLU_API="])

    res_dir = get_clang_resource_dir()
    if res_dir:
        filtered.append(f"-I{res_dir}/include")

    return filtered

file_cache = {}

def get_line_from_file(file_path, line_number):
    if file_path not in file_cache:
        with open(file_path, 'r', errors='ignore') as f:
            file_cache[file_path] = f.readlines()

    # line_number w Clangu jest od 1, w liście od 0
    idx = line_number - 1
    if 0 <= idx < len(file_cache[file_path]):
        return file_cache[file_path][idx]
    return ""

def has_macro_above(node, macro_name):
    """Sprawdza czy w liniach nad węzłem znajduje się makro (regex)"""
    file_path = node.location.file.name
    line_idx = node.location.line

    # Sprawdzamy do 3 linii w górę (na wypadek spacji/komentarzy)
    for i in range(1, 4):
        line = get_line_from_file(file_path, line_idx - i)
        if re.search(rf"{macro_name}\s*\(", line):
            return True
    return False

def get_compilation_map():
    if not os.path.exists(DATABASE_FILE):
        print(f"Błąd: Brak {DATABASE_FILE}. Uruchom CMake.")
        return {}

    with open(DATABASE_FILE, 'r') as f:
        db_json = json.load(f)

    folder_map = {}
    for entry in db_json:
        directory = os.path.dirname(entry['file'])
        if directory not in folder_map:
            args = entry['arguments'] if 'arguments' in entry else shlex.split(entry['command'])
            folder_map[directory] = filter_args(args[1:-1])
    return folder_map

def get_macro_params(node, macro_name):
    """Zwraca listę parametrów z makra nad węzłem lub None jeśli brak makra"""
    file_path = node.location.file.name
    line_idx = node.location.line

    # Przeszukujemy 3 linie nad deklaracją
    for i in range(1, 4):
        line = get_line_from_file(file_path, line_idx - i)
        # Szukamy: NAZWA_MAKRA( parametry )
        match = re.search(rf"{macro_name}\s*\((.*?)\)", line)
        if match:
            params_str = match.group(1).strip()
            if not params_str:
                return []
            # Dzielimy po przecinku i czyścimy spacje
            return [p.strip() for p in params_str.split(",")]

    return None # Brak makra

def find_reflection_data(node, file_path):
    results = []

    for child in node.get_children():
        # Pomijaj pliki zewnętrzne
        if not child.location.file or child.location.file.name != file_path:
            continue

        # Szukamy Klas
        if child.kind == clang.cindex.CursorKind.CLASS_DECL and child.is_definition():
            # Sprawdzamy Regexem, czy nad klasą jest PLU_CLASS
            if has_macro_above(child, "PLU_CLASS"):
                print(f"   [OK] Znaleziono: {child.spelling}")

                params = get_macro_params(child, "PLU_CLASS")
                if params is not None:
                    print(f"        Reflection Params:")
                    print(f"        {params}")

                # Wyciągamy klasy bazowe z AST (to akurat Clang robi dobrze)
                bases = []
                for sub in child.get_children():
                    if sub.kind == clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
                        bases.append(sub.type.spelling)

                classPath = Path(child.location.file.name)
                relative = classPath.relative_to(GURU_PROJECT_ROOT)
                firstSubfolder = relative.parts[0]
                print(relative)

                class_info = {
                    "name": child.spelling,
                    "bases": bases,         # DODANO: naprawia KeyError
                    "filepath": child.location.file.name,
                    "Project": firstSubfolder,
                    "params": params,
                    "properties": []
                }



                # Szukamy pól
                for member in child.get_children():
                    if member.kind == clang.cindex.CursorKind.FIELD_DECL:
                        if has_macro_above(member, "PLU_PROPERTY"):
                            params = get_macro_params(member, "PLU_PROPERTY")
                            if params is not None:
                                print(f"        {member.spelling}: {params}")
                            class_info["properties"].append({
                                "name": member.spelling,
                                "type": member.type.spelling
                            })

                results.append(class_info)

        # Rekurencja dla namespace'ów
        results.extend(find_reflection_data(child, file_path))

    return results

def process_project():
    folder_map = get_compilation_map()
    if not folder_map: return

    index = clang.cindex.Index.create()
    all_reflection_data = []

    print(f"Rozpoczynanie skanowania w: {PROJECT_ROOT}")

    for root, dirs, files in os.walk(PROJECT_ROOT):
        if "cmake-build" in root or ".git" in root: continue

        for file in files:
            if file.endswith(('.h', '.hpp', '.cpp')):
                full_path = os.path.abspath(os.path.join(root, file))

                # Szybki filtr wstępny
                with open(full_path, 'r', errors='ignore') as f:
                    if "PLU_CLASS" not in f.read(): continue

                classPath = Path(full_path)
                print(file)
                relative = classPath.relative_to(GURU_PROJECT_ROOT)
                firstSubfolder = relative.parts[0]
                pathToProcessedList = os.path.join(OUTPUT_FILE, firstSubfolder, "ProcessedList.txt")

                # Upewnij się, że folder istnieje
                os.makedirs(os.path.join(OUTPUT_FILE, firstSubfolder), exist_ok=True)

                if os.path.exists(pathToProcessedList):
                    with open(pathToProcessedList, "r+") as f:
                        try:
                            data = json.load(f)
                        except json.JSONDecodeError:
                            data = {}
                        file_mod_time = EngineUtils.modify_date(full_path)
                        if data.get(file) == file_mod_time:
                            print(f"Skipping {file}")
                            continue
                        else:
                            data[file] = file_mod_time
                            f.seek(0)
                            f.write(json.dumps(data, indent=2))
                            f.truncate()
                else:
                    # Plik nie istnieje, tworzymy nowy
                    data = {file: EngineUtils.modify_date(full_path)}
                    with open(pathToProcessedList, "w") as f:
                        f.write(json.dumps(data, indent=2))




                print(f"Parsowanie: {file}...")
                args = folder_map.get(root, next(iter(folder_map.values())))
                tu = index.parse(full_path, args=args)

                #print any errors
                for diag in tu.diagnostics:
                    print(diag.severity, diag.location, diag.spelling)

                data = find_reflection_data(tu.cursor, full_path)
                if data:
                    all_reflection_data.extend(data)

    generate_code(all_reflection_data)

def generate_code(data):
    # Grupowanie klas według projektów dla plików Init
    project_groups = {}

    for cls in data:
        proj = cls["Project"]
        if proj not in project_groups:
            project_groups[proj] = []
        project_groups[proj].append(cls)

        # Ścieżki plików
        classGeneratedHeader = os.path.join(OUTPUT_FILE, proj, cls["name"] + ".generated.h")
        classGeneratedSource = os.path.join(OUTPUT_FILE, proj, cls["name"] + ".generated.cpp")
        os.makedirs(os.path.dirname(classGeneratedHeader), exist_ok=True)

        # --- GENEROWANIE .h ---
        with open(classGeneratedHeader, "w") as f:
            f.write("#pragma once\n")
            f.write("#include <PluEngine/Reflection/ReflectionBase.h>\n\n")
            # Makro musi mieć unikalną nazwę na klasę lub być generyczne bez średnika
            fcls: str = cls["name"]
            fcls = fcls.upper()
            f.write(f"#define REFLECTION_BODY_{fcls}() \\\n")
            f.write(f"    public: \\\n")
            f.write(f"        static Plu::TypeInfo* GetStaticClass(); \\\n")
            f.write(f"        virtual Plu::TypeInfo* GetClass() override; \\\n")
            f.write(f"    private: \\\n")
            f.write(f"        friend void Register_Reflection_{cls['name']}();\n")

        # --- GENEROWANIE .cpp ---
        with open(classGeneratedSource, "w") as f:
            f.write(f'#include "{cls["filepath"]}"\n')
            f.write(f'#include "{cls["name"]}.generated.h"\n\n')
            f.write(f"using namespace Plu;\n\n")

            # 1. Implementacja StaticClass
            f.write(f"TypeInfo* {cls['name']}::GetStaticClass() {{\n")
            f.write(f"    static TypeInfo* instance = nullptr;\n")
            f.write(f"    if (!instance) {{\n")
            f.write(f'        instance = new TypeInfo(sizeof({cls["name"]}), "{cls["name"]}");\n')
            if not cls["params"] or "Abstract" not in cls["params"]:
                f.write(f"        instance->Constructor = []() -> void* {{ return new {cls['name']}(); }};\n")
            f.write(f"    }}\n")
            f.write(f"    return instance;\n")
            f.write(f"}}\n\n")

            f.write(f"TypeInfo* {cls['name']}::GetClass() {{ return GetStaticClass(); }}\n\n")

            # 2. Definicja funkcji rejestrującej (To naprawia błąd linkowania!)
            f.write(f"void Register_Reflection_{cls['name']}() {{\n")
            f.write(f"    TypeInfo* info = {cls['name']}::GetStaticClass();\n")
            f.write(f"    TypeRegistry::GetInstance()->AddType(info);\n")
            for prop in cls["properties"]:
                # Tutaj dodajemy właściwości do TypeInfo
                f.write(f'    info->AddProperty("{prop["name"]}", &{cls["name"]}::{prop["name"]});\n')
            f.write(f"}}\n")

    # --- GENEROWANIE InitReflection.cpp DLA KAŻDEGO PROJEKTU ---
    for proj_name, classes in project_groups.items():
        init_path = os.path.join(OUTPUT_FILE, proj_name, "InitReflection.cpp")
        with open(init_path, "w") as f:
            f.write(f"// Generated Init for {proj_name}\n")
            f.write("#include <PluEngine/Reflection/ReflectionBase.h>\n\n")

            # Deklaracje extern, żeby kompilator wiedział o funkcjach w innych plikach .cpp
            for cls in classes:
                f.write(f"extern void Register_Reflection_{cls['name']}();\n")

            f.write(f"\nvoid PLU_API Init{proj_name}Reflection() {{\n")
            for cls in classes:
                f.write(f"    Register_Reflection_{cls['name']}();\n")
            f.write("}\n")

    print(f"Gotowe! Wygenerowano dane dla {len(data)} klas w {OUTPUT_FILE}")

if __name__ == "__main__":
    process_project()