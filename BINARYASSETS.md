# Binary Runtime Assets — Design Plan

Status: **plan only, nothing implemented.**

This document describes how to add a binary on-disk form for cooked runtime assets,
alongside the JSON form the editor already uses. It exists because scene loading is
now dominated by JSON parsing, and because the same mechanism pays off for every
other asset type, not just scenes.

The guiding decision: **JSON stays the source format in the editor** (diffable in git,
no workflow regression), and the binary form is a derived artifact produced when a
project is packaged for distribution.

---

## 1. Why: measurements

Scene loading was optimised in `b7d9151` (PIE entry, ~93x) and `04c9685`
(deserialisation, ~1.3x). Both commits moved cost out of the deserialiser, which left
the JSON parse itself as the dominant term — `04c9685` says as much: *"the parse now
dominates it — compacting the scene format is the next lever."*

Measured on a synthetic 1005-object scene in the engine's exact scene schema
(`typeName` / `fields[{name,value}]` / `worldComponents` / `children`), g++ -O2,
median of 9 runs:

| Load path | median ms | vs current |
|---|---|---|
| `parse dump(4)` from ifstream — **current** | 54.1 | 1.00x |
| `parse dump(4)` from a memory buffer | 54.6 | 0.99x |
| `parse dump()` compact from memory | 36.3 | 1.49x |
| `from_msgpack` | 49.9 | 1.09x |
| `from_cbor` | 42.6 | 1.27x |
| SQLite, blob per object, JSON payload | 32.0 | 1.69x |
| SQLite, blob per object, msgpack payload | 29.3 | 1.85x |
| SQLite, normalised, one row per property | 7.8 | 6.95x |
| **Flat binary + string table, no DOM** | **0.10** | **521x** |

File sizes for the same scene: `dump(4)` 4999 KB, compact JSON 1388 KB,
SQLite 1180-1580 KB, **flat binary 411 KB**.

The breakdown explains the shape of that table:

```
DOM build from compact JSON bytes    30.9 ms
walking an already-built DOM          2.1 ms   <- 6%
DOM deep copy (allocation only)      15.5 ms   <- floor for any nlohmann DOM
flat binary: read + walk, no DOM      0.09 ms
```

**94% of the cost is constructing the `nlohmann::json` DOM, not tokenizing text.**
That is why swapping the container format while still producing a DOM buys almost
nothing (msgpack: 1.09x — and nlohmann's binary readers are in fact slower than its
own JSON parser). The win comes from not building a DOM at all.

Two consequences worth recording, because they contradict reasonable intuitions:

- MessagePack/CBOR/BSON are **not** worth adopting here. They keep the DOM.
- SQLite is **not** the right tool for this. Its normalised layout is fast (6.95x)
  only because it also skips the DOM — and once that work is done, a flat binary
  gives 521x instead. SQLite's real strengths (partial reads, incremental writes,
  transactions, queries) are arguments about a streaming-world architecture, not
  about load speed. On a full rebuild its write path is slower than what we do today.

Scaling check at 5000 objects is linear and preserves the ordering
(285 ms → 36.2 ms → 0.63 ms).

### Scope of the win, honestly

This removes parsing from the profile for scene open and PIE entry. Total level load
time will still be dominated by `GetAssetData` pulling meshes and textures off disk and
uploading them to the GPU (`TypeTraits.h`, `TypeSerializer<TUsePointer<T>>::Deserialize`).
Do not expect a level to appear 500x faster; expect the parse term to disappear.

---

## 2. What already exists

A large part of the scaffolding is in place. This plan mostly connects existing pieces.

| Element | Location | State |
|---|---|---|
| `BinaryFileWriter` / `BinaryFileReader` | `DiskManager.h` | Done. RAII, POD read/write, `WriteArray`, length-prefixed `WriteString`, error reporting |
| `AssetLoaderType { JSON, Binary, Undefined }` | `AssetDescriptor.h` | Done, already branches asset loading |
| Binary asset header convention | `EngineAssetManager::LoadBinaryDescriptor` | magic `PLUA`, `version u32`, typeName, uuid. Versions: textures=1, mesh=2, skeleton=3 |
| Runtime discovery by `j`/`b` filename prefix | `EngineAssetManager::LoadAssetDescriptor` | **Cooking needs no discovery changes** |
| Cook step | `EngineAssetManager::PrepareAssetsForDistribution` | Exists; today it copies files verbatim and renames to `jAsset<uuid>` / `bAsset<uuid>` |
| Per-property function pointers emitted by the generator | `ReflectionGeneratorRegex.py` | One line per pointer; adding two more is a two-line change |

Note the version ceiling: `kMaxKnownAssetVersion` in `EngineAssetManager.cpp` gates
descriptor indexing. Any new binary payload version must raise it, or the asset stops
being indexed at all.

---

## 3. Layer 1 — binary `TypeSerializer` (the core)

### 3.1 Two new methods per specialisation

```cpp
static void WriteBinary(BinaryFileWriter& w, const void* value);
static void ReadBinary(DeserializationContext* dc, BinaryFileReader& r, void* outValue);
```

31 specialisations to cover (`bool`, `Int8`…`UInt64`, `float`, `double`, `String`,
`StringW`, `Path`, `PathW`, `PluUUID`, `glm::vec2/3/4`, `glm::quat`, `DynamicArray<T>`,
`TUsePointer<T>`, `TOwningPointer<T>`, `TClassPointer<T>`, `BoneRef`,
`CollisionProfileRef`, `GraphValue`, `IShaderUniform`, `TypeInfo*`).

Most are three lines. Four are not trivial:

- `DynamicArray<T>` — count followed by recursion.
- `TUsePointer<T>` to an asset — writes the UUID, reads back through
  `dc->assetManager->GetAssetData`, mirroring the existing JSON behaviour.
- `TClassPointer<T>` — type name resolved through the string table.
- `TypeInfo*` — the node itself, see below.

Properties with `IsPersistent == false` are skipped on both sides, as in the JSON path.

### 3.2 Key design decision: property names once per type, not once per object

This is the difference between a 7x result and a 500x result.

```
header:    magic 'PLUB' | formatVersion u32 | typeTableCount u32 | stringTableCount u32
strings:   [len u16, bytes]*                       <- type and property names, deduplicated
typeTable: [typeNameIdx u32, propCount u16, [propNameIdx u32]*]*
payload:   [typeTableIdx u32, [value]*, childCount u16, [node]*]*
```

At load time each type table is resolved **once per file** into a
`DynamicArray<PropertyInfo*>` — one `FindProperty` per property per type, not per
object. Reading an object is then a loop over property indices calling
`BinaryReadPtr(dc, reader, prop->GetPtr(obj))`: no string work, no hashing, no node
allocation. That is exactly the shape that measured 0.10 ms above.

Keeping the name table in the file (rather than relying on declaration order alone)
costs a few KB once per file and buys robustness for the "new engine build, old cooked
assets in a patch" case: a property that no longer exists is skipped instead of
desynchronising the whole stream.

### 3.3 Versioning: deliberately minimal

Because the binary form is derived from JSON, **payload backward compatibility is not
required**. A single `formatVersion`; a mismatch is a "recook assets" error, not a
migration path. This removes the most expensive part of maintaining such a format.
Record this next to the magic number so nobody starts writing migrators later.

### 3.4 Generator support

Two lines next to the existing `SerializePtr` / `DeserializePtr` emission in
`ReflectionGeneratorRegex.py`:

```
prop{Name}->BinaryWritePtr = TypeSerializer<{Type}>::WriteBinary;
prop{Name}->BinaryReadPtr  = TypeSerializer<{Type}>::ReadBinary;
```

plus the two matching fields on `PropertyInfo` in `ReflectionBase.h`.

This doubles as the **drift guard**: a property whose type has `Serialize` but no
`WriteBinary` fails the build immediately, rather than silently misbehaving at cook
time. It is the single most important thing keeping this plan correct over time, and
it is the reason two parallel serialisation paths are acceptable here (see §7).

---

## 4. Layer 2 — optional virtuals on `IAssetLoader`

Added in the style of the existing `DispatchAssetSave`: non-pure, defaulting to
"not supported", so no existing loader is forced to implement anything.

```cpp
// Runtime: read this asset from its cooked binary form. Default: not supported.
virtual bool LoadAssetDataBinary(TUsePointer<AssetDescriptor> assetDesc,
                                 TOwningPointer<IAssetData>* assetDataToPopulate,
                                 BinaryFileReader& reader, /* managers… */) { return false; }

#ifdef PLU_ENGINE_EDITOR_BUILD
// Cook: write this asset in the engine's binary form. Default: not supported.
virtual bool CookAssetToBinary(TUsePointer<AssetDescriptor> assetDesc,
                               BinaryFileWriter& writer, /* managers… */) { return false; }
#endif
```

### Three-level fallback in `PrepareAssetsForDistribution`

This is what makes the feature genuinely optional:

1. `loader->CookAssetToBinary(...)` returns `true` — done. For types that already own
   a hand-rolled format (static mesh, skeletal mesh, textures, skeleton).
2. Otherwise: **generic reflection-driven cook** through `TypeInfo` plus layer 1.
   This covers every asset that currently goes through `DeSerializeFromJSON` in
   `EngineAssetManager::LoadJSONAssetData` — materials, shaders, collision presets,
   animation graphs — **without writing a single dedicated cooker**.
3. Otherwise (no reflection for the type): copy the JSON verbatim as `jAsset<uuid>`,
   exactly as today.

Step 2 is the real multiplier. Coverage does not scale with the number of loaders
written; every reflected asset type is handled at once.

The read side mirrors this in `LoadBinaryAssetData`: loader → generic reflection
reader → error.

---

## 5. Layer 3 — scenes

Scenes do not fit the shape above. The scene payload is not an `IAssetData`:
`SceneAssetHandler::LoadAssetData` reads only the `uuid` from the file and registers a
`SceneInfo`, while the actual contents are loaded by `SceneManager::LoadSceneFromFile`
when a scene is opened. So scenes need their own pair:

- `SceneManager::SaveSceneBinary(path)` — bypasses `SerializeActiveScene`, writes the
  object → worldComponents → children → components tree through the layer 1 writer.
- `SceneManager::LoadSceneFromBinary(sceneWorld, reader)` — mirrors
  `LoadSceneFromJson`: same `SpawnGameObjectUnnamed`, same single
  `DeserializationContext` per scene load, same `ResolvePendingAttachments` at the end.
- `attachment` (parentUuid / componentName / socket) and name-based component matching
  keep their current semantics; only the source of the bytes changes.

This is the one place where deserialisation *logic* is genuinely duplicated rather than
just per-type primitives. Keep both paths adjacent in the same file so divergence is
visible in review.

---

## 6. Implementation order

### Phase A — hardening and a measured baseline

Before widening the binary surface, the existing binary entry point needs to be safe,
because this plan multiplies the number of binary inputs.
`EngineAssetManager::LoadBinaryDescriptor` currently:

- does not check the result of `fopen` before passing the handle to `fread` — an
  unopenable file is undefined behaviour;
- reads `typeLength` from the file and then does `new char[typeLength + 1]` with no
  bound check — a corrupt or hand-crafted file yields a huge allocation or worse;
- does not check any `fread` return value.

The rule to adopt for the format generally: **validate every count and length read from
a file against the file size before allocating.** Moving this function onto the existing
`BinaryFileReader` handles most of it, since that class already reports short reads.

Also capture a baseline for `LoadSceneFromFile Parse` on a real scene using
`--profiler-export-after` (added in `04c9685`), so phase D has something to compare against.

### Phase B — layer 1

`WriteBinary` / `ReadBinary` across the 31 specialisations, two lines in the generator,
two fields on `PropertyInfo`. Nothing consumes it yet; the build must stay green.
Test: round-trip each property type in isolation.

### Phase C — layer 2

Generic reflection cook/read, the two `IAssetLoader` virtuals, and the fallback chain in
`PrepareAssetsForDistribution`. Acceptance test: package a project, run the runtime,
confirm everything loads as it did from JSON. The win on materials, shaders and
animation graphs lands here.

### Phase D — scenes

Where the measured 54 ms → ~0.1-1 ms parse improvement on a 1000-object scene lands.

### Phase E — optional

Dedicated `CookAssetToBinary` implementations for types where the generic path is
suboptimal.

Phases B and C are independent of whether D ever happens, and D is not worth starting
without B.

---

## 7. Things to know up front

**Obfuscation.** A binary format raises the barrier to casual inspection of shipped
scenes, but it is not protection: the format is recoverable from the engine binary, and
the property-name table described in §3.2 documents the structure for anyone who looks.
Treat it as a welcome side effect of a performance change, not as a security measure.

**Padding and endianness.** `SkeletalMeshImporter` already decodes field by field on
purpose, so the on-disk format does not depend on struct padding or `sizeof`
(see `04c9685`). The same discipline applies here: never `Write(wholeStruct)` for
composite types, even though `BinaryFileWriter::Write<T>` will happily accept them
because `is_trivially_copyable_v` passes.

**Distribution size** drops as a side effect: 4999 KB → 411 KB for the benchmark scene,
mostly from the string table and from not repeating field names per object.

**Two parallel paths, by choice.** An alternative design replaces
`const nlohmann::json&` in `TypeSerializer::Deserialize` with a reader abstraction so
one code path serves both formats. That was considered and rejected: the duplication is
bounded (per-type primitives, not logic, outside of §5), the JSON path keeps working
untouched, and the generator-level guard in §3.4 solves the drift problem that was the
main argument for a shared abstraction.

**Unrelated but adjacent.** `DiskManager::SaveJsonInternal` still uses `dump(4)`.
Switching to `dump()` is a free 1.49x on editor-side save/load, but it costs
readable git diffs for scene and asset files. Independent decision, not part of this plan.

---

## 8. Out of scope

This plan does not touch SQLite, does not change the editor-side format, does not
modify the working binary paths for meshes/textures/skeletons, and does not introduce a
reader abstraction over `nlohmann::json`.
