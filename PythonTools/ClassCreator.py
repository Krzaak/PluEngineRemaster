import argparse
from pathlib import Path
import os

argParser = argparse.ArgumentParser(prog="Class Creator", description="Creates classes for PluEngine")

argParser.add_argument("-p", "--project")
argParser.add_argument("-n", "--name")
argParser.add_argument("-P", "--path")

SCRIPT_DIR: Path = Path(os.path.dirname(os.path.abspath(__file__)))
ENGINE_DIR: Path = Path()

def isEngineDir(path: Path) -> bool:
    engineDirFound = os.path.exists(os.path.join(path, "Engine"))
    editorDirFound = os.path.exists(os.path.join(path, "Editor"))
    pluStlFound = os.path.exists(os.path.join(path, "PluSTL"))
    pythonToolsFound = os.path.exists(os.path.join(path, "PythonTools"))
    vcpkgJsonFound = os.path.exists(os.path.join(path, "vcpkg.json"))
    return engineDirFound and editorDirFound and pluStlFound and pythonToolsFound and vcpkgJsonFound

if isEngineDir(SCRIPT_DIR.parent):
    ENGINE_DIR = SCRIPT_DIR.parent
ENGINE_DIR = Path(os.path.join(ENGINE_DIR, "Engine"))
INCLUDE_DIR = Path(os.path.join(ENGINE_DIR, "include"))
SCRIPT_DIR = Path(os.path.join(ENGINE_DIR, "src"))

print(f"Script Dir: {SCRIPT_DIR}")
print(f"Include Dir: {INCLUDE_DIR}")

NewClassPath = str(input("Enter path for new class, default = PluEngine/\n"))
if NewClassPath == "":
    NewClassPath = "PluEngine/"
NewClassName = str(input("New Class name:"))
if NewClassName.strip == "":
    print("Error!")
    exit()    
NewClassCPP = os.path.join(SCRIPT_DIR, NewClassPath, NewClassName + ".cpp")
NewClassH = os.path.join(INCLUDE_DIR, NewClassPath, NewClassName + ".h")

os.makedirs(Path(NewClassCPP).parent, 511, True)
os.makedirs(Path(NewClassH).parent, 511, True)

print(NewClassCPP)
print(NewClassH)

with open(NewClassCPP, "w") as cpp:
    cpp.write(f"#include {'"'}{os.path.join(NewClassPath, NewClassName + ".h")}{'"'}")

with open(NewClassH, "w") as h:
    h.write("//\n")
    h.write(f"// Created by Plutex on YYYY-MM-DD.\n")
    h.write("//\n\n")
    h.write(f"#ifndef PLUENGINE_{NewClassName.upper()}_H\n")
    h.write(f"#define PLUENGINE_{NewClassName.upper()}_H\n")
    h.write('#include "PluEngine/Core.h"\n')
    h.write('#include "PluSTL_FWD.h"\n')
    h.write(f'#include "{NewClassName}.generated.h"\n\n')
    h.write(f"namespace Plu\n{{\n	class PLU_API {NewClassName} : public EngineObject\n{{\n")
    h.write(f"		REFLECTION_BODY_{NewClassName.upper()}()\n	public:\n")
    h.write(f"		{NewClassName}();\n")
    h.write(f"		virtual ~{NewClassName}() override;\n")
    h.write("	}\n		}")
