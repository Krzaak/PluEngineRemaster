import argparse
from pathlib import Path
import os

argParser = argparse.ArgumentParser(prog="Class Creator", description="Creates classes for PluEngine")

argParser.add_argument("-p", "--project")
argParser.add_argument("-n", "--name")
argParser.add_argument("-P", "--path")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_DIR: Path = Path()

def isEngineDir(path: Path) -> bool:
    engineDirFound = os.path.exists(os.path.join(path, "Engine"))
    editorDirFound = os.path.exists(os.path.join(path, "Editor"))
    pluStlFound = os.path.exists(os.path.join(path, "PluSTL"))
    pythonToolsFound = os.path.exists(os.path.join(path, "PythonTools"))
    vcpkgJsonFound = os.path.exists(os.path.join(path, "vcpkg.json"))
    return engineDirFound and editorDirFound and pluStlFound and pythonToolsFound and vcpkgJsonFound

if isEngineDir(SCRIPT_DIR):
    ENGINE_DIR = SCRIPT_DIR
else:
    if isEngineDir(os.path.pardir(SCRIPT_DIR)):
        ENGINE_DIR = os.path.pardir(SCRIPT_DIR)

