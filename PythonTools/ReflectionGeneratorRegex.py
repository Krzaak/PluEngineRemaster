import argparse
import os
import re
import sys
from enum import Enum
from typing import List
from pathlib import Path


class ClassType(Enum):
    CLASS = 1
    STRUCT = 2
    ENUM = 3
    INTERFACE = 4
    UNKNOWN = 5

# --- KONFIGURACJA ŚCIEŻEK ---
ScriptDir = os.path.dirname(os.path.abspath(__file__))
ProjectToScanDir = os.path.dirname(ScriptDir)
ProjectDir = ProjectToScanDir
OutputDir = os.path.join(ProjectDir, "ReflectionCache")
QuietMode: bool = False
ForceMode: bool = False

argParser = argparse.ArgumentParser(prog="Reflection Generator", description="Generates reflection data for PluEngine with Regex")

argParser.add_argument("-q", "--quiet", action="store_true")
argParser.add_argument("-p","--project")
argParser.add_argument("-F","--force", action="store_true", help="Ignore ProcessedList.txt?")
args = argParser.parse_args()
QuietMode = bool(args.quiet)
ForceMode = bool(args.force)

FilesToProcess: List[Path] = []

def ScanProject():
    print(f"Scanning Project at {ProjectToScanDir} ...")

    for dirpath, dirnames, files in os.walk(ProjectToScanDir):
        for file in files:
            if file.endswith(".h") and not file.endswith("generated.h"):
                with open(os.path.join(dirpath, file), "r") as f:
                    lines = f.readlines()
                    for line in lines:
                        line = line.strip()
                        if line.startswith("PLU_CLASS") or line.startswith("PLU_STRUCT"):
                            FilesToProcess.append(Path(dirpath, file))
    print(f"Found {len(FilesToProcess)} files to process")

classPattern = r"class\s+(?:[A-Z_]+\s+)?(\w+)(?:\s*:\s*(.*))?"
structPattern = r"struct\s+(?:[A-Z_]+\s+)?(\w+)(?:\s*:\s*(.*))?"

classesFound = []
structsFound = []
def ProcessFile(file: Path):
    print(f"Processing {file} ...")
    with open(file, "r") as f:
        lines = f.readlines()
        currentClass = ""
        for idx, line in enumerate(lines):
            #We go line by line and keep track of all access specifiers and other stuff that we encounter
            line = line.strip()
            if line.startswith("PLU_CLASS") or line.startswith("PLU_STRUCT"):
                match = re.match(classPattern, lines[idx + 1].strip())
                if match:
                    currentClass = {
                        "name": match.group(1),
                        "bases": match.group(2)
                    }
                    classesFound.append(currentClass)

if __name__ == "__main__":
    ScanProject()
    print("Processing...")
    for file in FilesToProcess:
        ProcessFile(file)
    for clas in classesFound:
        print(f"{clas['name']} -> {clas['bases']}")