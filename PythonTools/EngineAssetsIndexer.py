import json
import sys
from pathlib import Path
import os



startDir = os.getcwd()
startDir = Path(os.path.join(startDir, 'EngineAssets'))
print(startDir)
if not os.path.isdir(startDir):
    sys.exit("Engine Assets directory not found at {}".format(startDir))

assets = []

for root, dirs, files in os.walk(startDir):
    for file in files:
        if file.endswith(".pluasset"):
            with open(os.path.join(root, file), 'rb') as f:
                j = json.load(f)
                if "typeName" in j:
                    print(j["typeName"])
                if "fields" in j:
                    for field in j["fields"]:
                        if field["name"] == "Uuid":
                            print(field["value"])
                            filenameNoExt = Path(file).stem
                            assets.append({
                                "name": filenameNoExt,
                                "uuid": field["value"]})

with open(os.path.join(startDir.parent.absolute(), 'ReflectionCache', "Engine", "EngineAssets.h"), 'w') as f:
    f.write("#ifndef ENGINE_ASSETS_H\n")
    f.write("#define ENGINE_ASSETS_H\n")
    f.write("#include \"PluEngine/Core.h\"\n")
    f.write("namespace Plu {\n")
    f.write("   namespace EngineAssets {\n")
    for asset in assets:
        f.write(f"      constexpr UInt64 {asset["name"]} = {asset["uuid"]};\n")
    f.write("   }\n}\n")
    f.write("#endif // ENGINE_ASSETS_H\n")