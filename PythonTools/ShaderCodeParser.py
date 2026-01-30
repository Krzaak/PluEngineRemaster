import argparse
from pathlib import Path
import os

parser = argparse.ArgumentParser()

parser.add_argument("--project")

args = parser.parse_args()

projectPath = Path(args.project)

print(f"Parsing shaders at: {projectPath}/Shaders")

projectShadersPath = os.path.join(projectPath, "Shaders")

foundShaders: list[Path] = []

for subdir, dirs, files in os.walk(projectShadersPath):
    for file in files:
        shaderPath = os.path.join(subdir, file)
        if shaderPath.endswith((".frag", ".vert")):
            foundShaders.append(Path(shaderPath))

for shader in foundShaders:
    print(f"{shader}")

