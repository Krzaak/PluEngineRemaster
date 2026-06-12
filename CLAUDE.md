# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Two build targets exist: **Editor** (PLU_BUILD_EDITOR=ON) and **Runtime** (PLU_BUILD_EDITOR=OFF). Use CMake presets:

```bash
# Configure
cmake --preset PluDebugLinux-Editor     # Editor debug build → cmake-build-debug-editor/
cmake --preset PluDebugLinux-Runtime    # Runtime debug build → cmake-build-debug-runtime/

# Build
cmake --build --preset PluDebugLinux-Editor
cmake --build --preset PluDebugLinux-Runtime
```

Other presets follow the pattern `Plu{Debug|Release|RelDbg}{Linux|Windows}-{Editor|Runtime}`.

## Reflection Code Generation

**Critical**: any time you add or change `PLU_CLASS`, `PLU_STRUCT`, `PLU_ENUM`, `PLU_PROPERTY`, or `PLU_FUNCTION` annotations in a header, the reflection generator must be re-run before building. It reads `compile_commands.json` via libclang and writes `*.generated.h` files into `ReflectionCache/`.

```bash
# Run from project root, requires cmake-build-debug/ to exist with compile_commands.json
cd PythonTools && python ReflectionGeneratorRegex.py
# Single file mode (faster during iteration):
python ReflectionGeneratorRegex.py --file path/to/MyClass.h
# Force-reprocess everything:
python ReflectionGeneratorRegex.py --force
```

The Python venv used by the build system lives at `Python/venv-linux` (Linux) or `Python/venv-windows` (Windows). Requirements are auto-installed by CMake if `Python/requirements.txt` changes.

## Architecture Overview

### Module structure

- **`LibEngine/`** — the engine shared library. Public API lives under `LibEngine/include/PluEngine/`. Implementation in `LibEngine/src/`.
- **`Editor/`** — editor application; only compiled when `PLU_BUILD_EDITOR=ON`. Defines panels, viewports (scene, material, static mesh, shader, texture), and editor-specific managers.
- **`Runtime/`** — standalone runtime app; compiled when `PLU_BUILD_EDITOR=OFF`.
- **`PluSTL/`** — custom containers and smart pointers used everywhere instead of `std::`.
- **`PythonTools/`** — offline tooling: `ReflectionGeneratorRegex.py`, `ClassCreator.py`, `EngineAssetsIndexer.py`, `ShaderCodeParser.py`.
- **`ThirdParty/`** — vendored: ImGui, glad, glm, nlohmann/json, pybind11, spdlog, stb_image. Everything else comes from vcpkg.

### Application lifecycle

`Application` (abstract, `LibEngine/include/PluEngine/Application.h`) is subclassed by `EditorApp` and `RuntimeApp`. It owns an `EngineObjectManager` and an `ApplicationInfo` struct that holds `TUsePointer<>` references to every major subsystem (Renderer, SceneManager, ShadersManager, AssetsManager, InputManager, PythonManager, GameClient, …). `Application::Run()` drives the main loop.

### Object model

Every engine object inherits from `EngineObject`. Lifetime is managed by `EngineObjectManager`, a slot-map that issues `EngineObjectHandle` (index + generation) values. Objects are never accessed by raw pointer — always via the handle or through one of the two smart pointers:

- `TOwningPointer<T>` — reference-counted owning pointer (analogous to `shared_ptr`). When owning count drops to zero the object is deleted.
- `TUsePointer<T>` — non-owning observer (analogous to `weak_ptr`). Does not extend lifetime; becomes null-like when the owner is gone.

Both are backed by a shared `ControlBlock`. **Never use raw pointers or `std::shared_ptr` for engine objects.**

### Reflection system

Classes annotate themselves with macros defined in `LibEngine/include/PluEngine/Core.h`:

| Macro | Purpose |
|---|---|
| `PLU_CLASS(...)` | Mark a class for reflection |
| `PLU_STRUCT(...)` | Mark a struct |
| `PLU_ENUM(PyNamespace=Plu)` | Mark an enum — `PyNamespace` is required, omitting it causes compile errors |
| `PLU_PROPERTY(...)` | Expose a field (serialisation, editor UI) |
| `PLU_FUNCTION(...)` | Expose a method |

Optional specifiers inside the parentheses: `Abstract`, `PyExport` (expose to Python), `PyDerive` (allow Python subclassing), `PyOverride` (on functions — can be overridden from Python), `PyNotCallable`.

`ReflectionGeneratorRegex.py` parses these with libclang and generates `ClassName.generated.h` (included inside the class definition via `REFLECTION_BODY_CLASSNAME()`). `TypeInfo` / `PropertyInfo` structs are registered globally in `TypeRegistry`.

Serialisation and editor widgets are driven by specialisations of `TypeSerializer<T>` (see `ReflectionBase.h`).

### Scene & gameplay

- `SceneWorld` owns a `GameHashMap<UInt64, TOwningPointer<GameObject>>` and a `PhysicsWorld`. It is responsible for spawning/destroying objects, ticking, and coordinating with the `Renderer`.
- `GameObject` holds transform (location/rotation/scale), a compound physics shape, a UUID, and two component lists: `GameObjectComponent` (non-spatial logic) and `WorldComponent` (spatial, forms a transform hierarchy).
- `GameMode` (a `GameObject` subclass) sets the active `Controller` and `Puppet` classes. One `GameMode` lives per `SceneWorld`.
- `Controller → Puppet` is the player input chain.

### Renderer

`Renderer` (`LibEngine/include/PluEngine/Renderer/Renderer.h`) runs its own thread (`mRenderThread`). It holds a list of `IRenderable*` objects, one active `IRendererCamera`, a main `FrameBuffer`, and a shadow-map `FrameBuffer` for directional lights. In editor builds a separate editor shadow framebuffer also exists (`#ifdef PLU_ENGINE_EDITOR_BUILD`).

### PluSTL containers

Always use PluSTL types, not `std::` equivalents:

| PluSTL | Replaces |
|---|---|
| `DynamicArray<T>` | `std::vector<T>` |
| `GameHashMap<K,V>` | `std::unordered_map<K,V>` |
| `HashSet<T>` | `std::unordered_set<T>` |
| `String` | `std::string` |
| `Path` | `std::filesystem::path` |

All types are forward-declared in `PluSTL/PluSTL_FWD.h`.

### Type aliases (PluTypes.h)

`Vec2/3/4`, `IVec2/3/4`, `Matrix4`, `Quaternion` — all glm wrappers. `JSON` = `nlohmann::json`.
Integer aliases: `UInt8/16/32/64`, `Int8/16/32/64` (from `Core.h`).

**Namespace uwaga**: `Vec2`, `Vec3`, `Vec4`, `IVec2/3/4`, `Matrix4`, `Quaternion` są w **global namespace**, nie w `Plu::`. Podobnie `String`, `Path`, `PathW`, `DynamicArray`, `GameHashMap` itp. z PluSTL. Tylko klasy silnika (`EngineObject`, `ShaderProgram`, `IShaderCode`, …) oraz smart pointery (`TOwningPointer`, `TUsePointer`) żyją w `Plu::`. W kodzie poza blokiem `namespace Plu { }` (np. w `.cpp` używającym stylu `Plu::ClassName::Method`) typy matematyczne i STL używa się bez prefixu, a typy silnika z prefixem `Plu::`.

### Python scripting

Python runs inside the same process via pybind11. Types annotated `PyExport` / `PyDerive` are registered with `TypeRegistry` and exposed to Python. The editor manages a separate Python venv for project scripts (`~/.local/share/PluEngine/PythonEnv` on Linux). PyArmor is used to obfuscate scripts in distribution builds.

### Editor structure

Editor panels live in `Editor/Panels/` (base infrastructure) and `Editor/DefinedPanels/` (concrete panels). Viewports (render-to-texture windows) live in `Editor/DefinedViewports/`. Editor-specific managers override or extend engine managers and are in `Editor/Managers/`.
