import argparse
from pathlib import Path
import os
import re

parser = argparse.ArgumentParser()

parser.add_argument("--project")
parser.add_argument("--engine")
parser.add_argument("--file")

args = parser.parse_args()

if not args.project or not args.engine:
    parser.print_help()
    exit(1)
projectPath = Path(args.project)
enginePath = Path(args.engine)
filePath: Path = Path("")
fileMode: bool = False
if args.file:
    filePath = args.file
    fileMode = True
    print(f"File: {filePath}")

print(f"Parsing shaders at: {projectPath}/Shaders")
print(f"Parsing Engine shaders at: {enginePath}/Shaders")

engineShadersPath = os.path.join(enginePath, "Shaders")
projectShadersPath = os.path.join(projectPath, "Shaders")
projectCachePath = os.path.join(projectPath, "Cache")

foundShaders: list[Path] = []

if not fileMode:
    for subdir, dirs, files in os.walk(projectShadersPath):
        for file in files:
            shaderPath = os.path.join(subdir, file)
            if shaderPath.endswith((".frag", ".vert")):
                foundShaders.append(Path(shaderPath))
                print(f"Shader {shaderPath}")
    for subdir, dirs, files in os.walk(engineShadersPath):
        for file in files:
            shaderPath = os.path.join(subdir, file)
            if shaderPath.endswith((".frag", ".vert")):
                foundShaders.append(Path(shaderPath))
                print(f"Engine Shader {shaderPath}")
else:
    foundShaders.append(Path(filePath))

uniformPattern = r'uniform\s+(\w+)\s+(\w+)(?:\[(\d+)\])?\s*;'

engineOnlyUniforms = [
    {"mat4", "model"},
    {"mat4", "view"},
    {"mat4", "projection"},
    {"vec3", "cameraPos"},
    {"vec4", "dirLightColor"},
    {"vec3", "dirLightDir"},
    # Uniformy cieni kaskadowych (CSM) sterowane przez silnik w Renderer::RenderSnapshot —
    # nie są parametrami materiału. Tablicowe i tak odpadają na renderze (ArraySize != 0),
    # ale cascadeCount to skalar, który bez tego wpadał do parametrów materiału i nadpisywał
    # globalną wartość zerem (gasząc cienie).
    {"int", "cascadeCount"},
    {"sampler2D", "cascadeShadowMaps"},
    {"mat4", "cascadeLightSpaceMatrices"},
    {"float", "cascadeSplitDistances"},
]

for shader in foundShaders:
    with open(shader, "r", encoding="utf-8") as s:
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

