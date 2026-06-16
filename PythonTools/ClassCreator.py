# Edited by WojanPlayTV on 28.02.2026
# Final touches by Plutex on 26.03.2026
import argparse
import os
from colorama import Fore, Back, Style

argParser = argparse.ArgumentParser(prog="Class Creator", description="Creates classes for PluEngine")

argParser.add_argument("-p", "--project", help="path to folder inside project")
argParser.add_argument("-n", "--name", help="name of created class")
argParser.add_argument("-P", "--path", help="path to project directory")

# argParser.print_help()
args = argParser.parse_args()
use_args = any((args.project, args.name, args.path))

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_DIR: str = ''

# Checking if it is the engine directory
def isEngineDir(path: str) -> bool:
    checks = ("Engine", "Editor", "PluSTL", "PythonTools", "vcpkg.json")
    if os.path.isdir(path):
        return len(tuple(filter(lambda x: x in checks, os.listdir(path)))) == len(checks)
    return False

# Getting the Engine directory
if use_args:
    ENGINE_DIR = args.path
elif isEngineDir(SCRIPT_DIR):
    ENGINE_DIR = SCRIPT_DIR
else:
    if isEngineDir(os.path.join(SCRIPT_DIR, '..')):
        ENGINE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '..'))

# Setup for project directory paths
ENGINE_DIR = os.path.join(ENGINE_DIR, "Engine")
INCLUDE_DIR = os.path.join(ENGINE_DIR, "include")
SORCE_DIR = os.path.join(ENGINE_DIR, "src")

print(f"Sorce Dir: {SORCE_DIR}")
print(f"Include Dir: {INCLUDE_DIR}")

if use_args:
    NewClassPath = args.project
    NewClassName = args.name
else:
    NewClassPath = input("Enter path for new class, default = PluEngine/\n")
    NewClassName = str(input("New Class name: "))

if not NewClassPath:
    NewClassPath = "PluEngine/"
if not NewClassName.strip():
    exit("Error!")

def Create_Files(NewClassName:str, NewClassPath:str):

    def Print_Class_Creation_Options():
        print(Fore.RED + Back.BLUE + "1. Class with PLU_API and PLU_CLASS() macro" + Style.RESET_ALL)
        print(Back.RED + "2. Class with PLU_API" + Style.RESET_ALL)
        print(Fore.RED + Back.BLUE + "3. Class with PLU_CLASS() macro" + Style.RESET_ALL)

    def Create_Class_In_Header(writer):
        Print_Class_Creation_Options()
        option = int(input("Enter option index:"))
        if option < 1 and option > 3:
            print("Error!")
            exit()
        writer.write('#include "PluEngine/Core.h"\n#include "PluEngine/PluTypes.h"')
        if option == 1 or option == 3:
            writer.write(f'#include "{NewClassName}.generated.h"')
        if option == 1:
            writer.write(f"PLU_CLASS()\nclass PLU_API {NewClassName} : public EngineObject")
        elif option == 2: 
            writer.write(f"class PLU_API {NewClassName} : public EngineObject")
        elif option == 3:
            writer.write(f"PLU_CLASS()\nclass {NewClassName} : public EngineObject")
        writer.write("\n{\n\n")
        writer.write("}\n")
    H_file_path = os.path.join(INCLUDE_DIR, NewClassPath, NewClassName + '.h')
    CPP_file_path = os.path.join(SORCE_DIR, NewClassPath,NewClassName + '.cpp')
    # h file in INCLUDE folder
    os.makedirs(os.path.dirname(H_file_path), 511, True)
    with open(H_file_path, 'w') as writer:
        writer.write(f"#ifndef PLUENGINE_{NewClassName.upper()}_H\n")
        writer.write(f"#define PLUENGINE_{NewClassName.upper()}_H\n\n")
        Create_Class_In_Header(writer)
        writer.write(f"\n#endif")

    # cpp file in SRC folder
    os.makedirs(os.path.dirname(CPP_file_path), 511, True)
    with open(CPP_file_path, 'w') as writer:
        writer.write(f"#include \"{NewClassPath}/{NewClassName}.h\"")

Create_Files(NewClassName, NewClassPath)

