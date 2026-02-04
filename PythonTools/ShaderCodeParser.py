import argparse
from pathlib import Path
import os
import re

parser = argparse.ArgumentParser()

parser.add_argument("--project")

args = parser.parse_args()

projectPath = Path(args.project)

print(f"Parsing shaders at: {projectPath}/Shaders")

projectShadersPath = os.path.join(projectPath, "Shaders")
projectCachePath = os.path.join(projectPath, "Cache")

foundShaders: list[Path] = []

for subdir, dirs, files in os.walk(projectShadersPath):
    for file in files:
        shaderPath = os.path.join(subdir, file)
        if shaderPath.endswith((".frag", ".vert")):
            foundShaders.append(Path(shaderPath))

uniformPattern = r'uniform\s+(\w+)\s+(\w+)(?:\[(\d+)\])?\s*;'

engineOnlyUniforms = [
    {"mat4", "model"},
    {"mat4", "view"},
    {"mat4", "projection"}
]

for shader in foundShaders:
    with open(shader, "r") as s:
        lines = s.readlines()
        fullFile: str = ""
        for line in lines:
            fullFile += line
        uniforms = re.findall(uniformPattern, fullFile)
        if not os.path.exists(os.path.join(projectCachePath, "ShaderParse/")):
            os.makedirs(os.path.join(projectCachePath, "ShaderParse/"))
        with open(os.path.join(projectCachePath, "ShaderParse", shader.name + ".txt"), "w") as r:
            for uniform in uniforms:
                if {uniform[0], uniform[1]} not in engineOnlyUniforms:
                    r.write(f"{uniform[0]} {uniform[1]} {uniform[2]}\n")

