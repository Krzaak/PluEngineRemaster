# HELPERS.md

Spis funkcji i makr pomocniczych (helperów) dostępnych w silniku. Zanim napiszesz
własny util, sprawdź czy nie ma już gotowego tutaj.

> **WAŻNE:** Ten plik trzeba aktualizować za każdym razem, gdy dodajesz, usuwasz
> lub zmieniasz funkcję/makro pomocnicze. Patrz sekcja [Utrzymanie](#utrzymanie).

---

## Math / Transform — `PluEngine/PluUtils.h` (`namespace Plu`)

Wszystkie rotacje przyjmowane są w **stopniach** jako `Vec3` (pitch=X, yaw=Y, roll=Z),
wewnętrznie konwertowane na radiany.

| Funkcja | Opis |
|---|---|
| `Vec3 GetForwardVector(Vec3 rot)` | Znormalizowany wektor "do przodu" (`0,0,-1`) dla danej rotacji. |
| `Vec3 GetRightVector(Vec3 rot)` | Znormalizowany wektor "w prawo" (`1,0,0`). |
| `Vec3 GetUpVector(Vec3 rot)` | Znormalizowany wektor "w górę" (`0,1,0`). |
| `double ClampD(double v, double min, double max)` | Clamp dla `double` (reflektowane, `PLU_FUNCTION`). |
| `float ClampF(float v, float min, float max)` | Clamp dla `float`. |
| `int ClampI(int v, int min, int max)` | Clamp dla `int`. |
| `float ClampAngle(float angle, float min, float max)` | Normalizuje kąt do `(-180,180]`, potem clampuje. |
| `double LerpD(double v, double target, double alpha)` | Interpolacja liniowa dla `double` (reflektowane, `PLU_FUNCTION`). `alpha` **nie** jest clampowana — poza `[0,1]` ekstrapoluje. |
| `float LerpF(float v, float target, float alpha)` | Lerp dla `float`. |
| `int LerpI(int v, int target, float alpha)` | Lerp dla `int` — wynik zaokrąglany do najbliższej liczby całkowitej. |
| `Vec3 LerpVec3(Vec3 v, Vec3 target, float alpha)` | Lerp po składowych dla `Vec3`. |
| `LerpClampedD/F/I/Vec3(v, target, alpha)` | To samo co `Lerp*`, ale `alpha` jest najpierw clampowana do `[0,1]` — wynik nigdy nie wychodzi poza zakres `v..target`. |
| `Vec3 GetLookAtRotatorDegrees(const Vec3& eye, const Vec3& target)` | Rotator (w stopniach) patrzący z `eye` na `target`. |
| `Vec3 GetRotatedPointWithRadius(const Vec3& center, float radius, float angleDeg, const Vec3& axis)` | Punkt na okręgu o promieniu `radius` wokół `center`, obrócony o `angleDeg` wokół `axis`. |
| `Vec3 GetSphericalOrbitPoint(const Vec3& center, float radius, float yawDeg, float pitchDeg)` | Punkt na sferze orbitalnej (yaw/pitch) — przydatne dla kamer orbitalnych. |
| `void NormalizeVec3Rotation(Vec3* vec)` | Normalizuje każdą oś rotacji do zakresu `[0,360)`. |
| `Vec3 GetLocationFromMatrix(const Matrix4& m)` | Translacja z macierzy transformacji (kolumna `m[3]`). |
| `Vec3 GetScaleFromMatrix(const Matrix4& m)` | Skala z macierzy (długości wektorów bazowych `m[0..2]`). |
| `Vec3 GetRotationFromMatrix(const Matrix4& m)` | Rotacja (Euler w **stopniach**, pitch=X/yaw=Y/roll=Z) z macierzy — baza znormalizowana skalą, `quat_cast` → `eulerAngles`. |
| `Vec4 PackUInt32ToColor(UInt32 id)` | Pakuje 32-bit id (np. obcięty UUID / indeks obiektu) do koloru RGBA `[0,1]` — bajt na kanał (R=bity 0-7 … A=bity 24-31). Do picking framebuffera. `inline`. |
| `UInt32 UnpackColorToUInt32(const Vec4& color)` | Odwrotność `PackUInt32ToColor` — odczytuje id z koloru (z zaokrągleniem, round-trip dokładny dla RGBA8). `inline`. |

`GetForwardVector`, `GetRightVector`, `GetUpVector` oraz funkcje `Clamp*`, `Lerp*` i `LerpClamped*` są oznaczone
`PLU_FUNCTION()` — są reflektowane i dostępne także z Pythona.

**Transform komponentu** (`GameObject/WorldComponent.h`, metody `WorldComponent`):

| Funkcja | Opis |
|---|---|
| `Matrix4 WorldComponent::GetWorldMatrix()` | Transform w przestrzeni świata (`parent * local`), cache'owany do najbliższej zmiany transformu. |
| `Matrix4 WorldComponent::GetMatrixRelativeToGameObject()` | Transform w przestrzeni **obiektu** — cały łańcuch relative transformów w górę, bez macierzy samego `GameObject`. Nie cache'owany. Tego (a nie `GetRelativeLocation/Rotation/Scale`) używa się, gdy komponent może być podpięty przez `AttachTo` pod inny komponent — patrz budowa compound shape'a w `PhysicsCompoundShape::Init`. |

**Component attachments** (`GameObject/WorldComponent.h`, methods on `WorldComponent`):

| Function | Description |
|---|---|
| `void AttachTo(WorldComponent* attachPoint, EAttachmentRule rule = KeepRelative)` | Attaches this component under `attachPoint` (both must belong to the same `GameObject`); `nullptr` puts it back directly under the object. Rejects self-attachment and descendant attachment (cycle) with an error log. Moves the owning pointer between attachment lists, invalidates the world matrix of the whole subtree and marks the owner's collision dirty. |
| `void Detach(EAttachmentRule rule = KeepRelative)` | `AttachTo(nullptr, rule)`. |
| `void SnapToAttachParent(bool keepScale = false)` | Clears the relative transform, so the component sits on its attach point — or on the owning object's origin when it has none (hence no „is attached" guard, unlike the `GameObject` version). In the editor: right-click a row in the Inspector's component tree. |
| `bool IsAttachedTo(WorldComponent* component)` | True when `component` is this component's parent, grandparent, … Used by the cycle guard and by the editor's drag&drop target test. |
| `TUsePointer<WorldComponent> GetParentComponent()` | Attach point, or null when the component hangs directly off the `GameObject`. |
| `DynamicArray<TUsePointer<WorldComponent>> GetChildren()` | Components attached directly under this one (one level). |

`EAttachmentRule::KeepRelative` leaves the relative transform alone (component snaps into the new parent's space); `KeepWorld` recomputes it so the component stays put in the world — that is what the editor's Inspector drag&drop uses; `SnapToTarget` zeroes it (sits exactly on the parent/socket). Whole-object views (physics, ticking, `GetComponentByClass`) go through `GameObject::GetObjectWorldComponents()`, which flattens the attachment tree; `GetDirectlyAttachedWorldComponents()` returns only the roots (serialization writes children nested under them).

**Object attachments** (`GameObject/GameObject.h`, methods on `GameObject`) — UE's `AActor::AttachToComponent`:

| Function | Description |
|---|---|
| `void AttachToComponent(WorldComponent* parent, const String& socket = "", EAttachmentRule rule = KeepRelative)` | Object rides another object's component, optionally a named socket (skeletal mesh attach point). Rejects a null parent, its own components and cycles with an error log. |
| `void AttachToObject(GameObject* parent, EAttachmentRule rule = KeepRelative)` | Object rides another object's transform. PluEngine has no root component like `AActor`, so this is the plain object-to-object parenting the outliner uses. |
| `void DetachFromParent(EAttachmentRule rule = KeepWorld)` | Releases the attachment. |
| `void SnapToAttachParent(bool keepScale = false)` | Clears the relative transform (location/rotation zeroed, scale 1), so the object sits exactly on its attach point — same end state as attaching with `SnapToTarget`. `keepScale` resets placement only. No-op when unattached. In the editor: „Snap to parent" / „Snap (keep scale)" in the Inspector's Attachment section and in the Structure panel's context menu. |
| `bool IsAttached()` / `TUsePointer<WorldComponent> GetAttachParentComponent()` / `TUsePointer<GameObject> GetAttachParentObject()` / `const String& GetAttachSocketName()` | Current attachment. `GetAttachParentObject` answers for both link kinds (component's owner, or the directly attached-to object). |
| `DynamicArray<TUsePointer<GameObject>> GetAttachedObjects()` / `WorldComponent::GetAttachedObjects()` | Children, one level deep. Non-owning — a destroyed parent detaches its children, it does not destroy them. |
| `bool IsAttachedToObject(GameObject*)` | Walks the whole chain (both link kinds); the cycle guard for the attach calls. |
| `void AttachToSkeletalMeshComponent(SkeletalMeshComponent*, const String& attachPoint)` | Wrapper over `AttachToComponent(..., SnapToTarget)`, kept for existing content. |

**World vs relative transform on `GameObject`** — `GetObjectLocation/Rotation/Scale` and their setters are **world** (unchanged meaning; the setters fold a world value back into the parent's space when attached). `GetRelativeLocation/…` and `SetRelativeLocation/…` are the offset from the attach parent, which is what scene JSON stores. While unattached the two are the same value, and the world getters return the stored fields verbatim rather than decomposing a matrix. `GetObjectWorldMatrix()` = attach-parent frame (socket frame when a socket is set) × local, rebuilt lazily; invalidation is pushed by `MarkWorldMatrixForRegeneration()` down through components and attached objects, and — for bone sockets, which change with the pose and have no setter — by `RenderSnapshotBuilder::EvaluateSkeletalPose` after each pose rebuild.

## Ścieżki / system — `PluEngine/PluUtils.h` (`namespace Plu`)

| Funkcja | Opis |
|---|---|
| `PathW GetEngineResourcesDir()` | Katalog zasobów silnika (`PLU_PROJECT_ROOT` w dev, obok exe w dystrybucji). |
| `PathW GetExePath()` | Pełna ścieżka do bieżącego pliku wykonywalnego (Win/Linux). `inline`. |
| `Path GetSystemUserPath()` | Katalog domowy użytkownika (`HOME` / `USERPROFILE`). |

## Disk I/O — `PluEngine/Managers/DiskManager.h` (`namespace Plu`)

`DiskManager` (statyczne): `SaveJson(StringW, json)`, `LoadJson(PathW) -> optional<json>`, `SaveText(StringW, String) -> bool` (writes already-formatted text verbatim, e.g. CSV dumps).

Binarne pliki: **`BinaryFileWriter` / `BinaryFileReader`** — scoped (RAII), zamykają plik w destruktorze; `CloseFile()` ręcznie (zwraca `bool` sukcesu). Non-copyable, movable. 256 KB bufor stdio (`setvbuf`). Konstruktor lub `OpenFile()` przyjmuje `Path` **lub** `PathW`. `HasError()` sygnalizuje short write/read, a `GetLastError()` zwraca opis przyczyny (errno + diagnoza ścieżki: brak katalogu nadrzędnego, plik to katalog, read-only, pusty plik, truncated stream, brak miejsca przy flushu). Preferuj zamiast surowego `fopen`/`fwrite`.

| Metoda | Opis |
|---|---|
| `OpenFile(Path\|PathW)` / `CloseFile()` | Otwórz / zamknij (auto-close w dtorze). |
| `Write(const T&)` / `Read(T&)` | Pojedyncza wartość POD (trivially copyable). |
| `WriteArray(const T*, count)` / `ReadArray(T*, count)` | Ciągła tablica POD. |
| `Write(void*, size)` / `Read(void*, size)` | Surowe bajty. |
| `WriteString(String)` / `ReadString(String&)` | String z prefiksem długości (`UInt32` + bajty UTF-8). |
| `IsOpen()` / `HasError()` | Stan pliku / flaga błędu. |
| `GetLastError()` | `const String&` — czytelny powód ostatniego błędu (open/read/write/close); pusty gdy brak. |

## Rejestr assetów — ścieżki (editor-only) — `PluEngine/Assets/EngineAssetManager.h`

Assety są w rejestrze trzymane po UUID **i** po ścieżce (`mAssetPathMap`, `mAssetPathByUUIDMap`, `AssetDescriptor::AssetPath/AssetName`). Przesunięcie pliku na dysku bez aktualizacji rejestru zostawia martwe ścieżki, więc:

| Metoda | Opis |
|---|---|
| `void RelocateAssets(const Path& oldPath, const Path& newPath)` | Po zmianie nazwy / przeniesieniu pliku assetu **albo całego katalogu** przepisuje ścieżki w rejestrze (UUID bez zmian, więc referencje działają dalej). Sama nie rusza dysku — wołaj po udanym `std::filesystem::rename`. |
| `bool AnyAssetsUnderDirectory(const Path& directory) const` | Czy w katalogu (rekurencyjnie) siedzi choć jeden zarejestrowany asset. Używane m.in. do blokowania kasowania folderu z assetami. |

## Import meshy z Assimp — `PluEngine/Assets/AssetLoaders/Mesh/MeshProcessing.h` (`namespace Plu::MeshProcessing`)

Wspólny kod konwersji sceny Assimp → geometria silnika, używany przez importer static **i** skeletal mesha (nie duplikuj tego w nowych importerach). Packery zapisują atrybuty w formacie wierzchołka `Vertex` (patrz `SetupStaticMeshGL`).

| Funkcja | Opis |
|---|---|
| `UInt32 PackNormal(const Vec3&)` | Normalna → spakowane `10_10_10_2` (signed). |
| `UInt32 PackTangent(const Vec3&, float sign)` | Tangent + handedness (`sign` = ±1) → `10_10_10_2`. |
| `UInt16 PackUV(float)` | UV (clamp 0..1) → 16-bit unorm. |
| `UInt32 PackColor(const aiColor4D&)` | RGBA → spakowane `RGBA8`. |
| `glm::mat4 AssimpToGLM(const aiMatrix4x4&)` | Macierz Assimp → GLM (column-major). |
| `void EnsureAssimpLoggerAttached()` | Jednorazowo mostkuje logger Assimpa do logów silnika. |
| `template ProcessMeshGeometry<VertexT>(aiMesh*, DynamicArray<VertexT>& verts, DynamicArray<UInt32>& indices, UInt16& matIdx, float scale, bool flipUVs, const glm::mat4& transform, bool isMerging)` | Wypełnia atrybuty bazowego `Vertex` jednego mesha. `VertexT` może dziedziczyć po `Vertex` (np. `SkeletalVertex` — skinning dopisujesz osobnym przebiegiem). `isMerging` przesuwa indeksy o aktualny rozmiar bufora. |
| `template ProcessNode<MeshDataT>(aiNode*, const aiScene*, DynamicArray<MeshDataT>& meshes, float scale, bool flipUVs, bool merge, const glm::mat4& parentTransform, DynamicArray<String>& meshNames)` | Rekurencyjnie chodzi po hierarchii nodów, akumuluje transformy, produkuje `MeshDataT` per mesh (lub jeden scalony przy `merge`). `MeshDataT` musi mieć `.Vertices`/`.Indices`/`.MaterialIndex`. |

## Skeleton — `PluEngine/AssetTypes/Skeleton/Skeleton.h` (`namespace Plu`, metody `Skeleton`)

Kolejność palety = **DFS pre-order** po drzewie `RootNode`, licząc **tylko** węzły `SkeletonBone` (zwykłe `SkeletonNode` pomijane, ale schodzi się przez nie w dół). Ta kolejność jest tym, do czego odnoszą się `SkeletalVertex::BoneIndices` (indeks z importu, stabilny po (de)serializacji). Funkcje zwracają **kopie** przez `out`-wskaźnik (czyszczony na starcie, `null` ignorowany) — animacja modyfikuje kopie, nie psuje współdzielonego assetu.

`Skeleton::ImportScale` to jednorodna skala, w której rig został zapieczony przy imporcie (`SkeletalMeshImportOptions::Scale`) — macierze węzłów już ją niosą, pole jest zapisem *którą*. Przy imporcie na istniejący rig (`SkeletonToUse`) importer domyślnie bierze skalę **ze szkieletu**, nie z opcji, więc literówka w polu `Scale` nie wrzuci mesha/klipu do innej przestrzeni niż jego szkielet (rozbieżność = warning). Gdy plik źródłowy naprawdę ma inne jednostki niż plik riga (rig w cm, klip w metrach), odznacza się `SkeletalMeshImportOptions::UseSkeletonImportScale` i wtedy wygrywa wpisana wartość — to ona zna jednostki tego pliku. Szkielety sprzed wersji 3 formatu wczytują się z `1.0`.

| Funkcja | Opis |
|---|---|
| `void Skeleton::CreateBonePalette(DynamicArray<TOwningPointer<SkeletonBone>>* out) const` | Płaska paleta kopii kości, **index-aligned z `SkeletalVertex::BoneIndices`**. Kopie samodzielne (`Children` puste) — to bufor skinningu podawany do shadera. Filtruj sloty po `BoneWeights[i] > 0` (index 0 przy pustym slocie ≠ prawdziwa kość 0). |
| `void Skeleton::CreateNodePalette(DynamicArray<TOwningPointer<SkeletonNode>>* out) const` | Paleta kopii **wszystkich** węzłów (kości i zwykłych) w DFS pre-order, z **zachowaną hierarchią** (`Children` na kopiach). Animowalne drzewo robocze do liczenia transformów globalnych; `out[0]` = kopia roota. |
| `Matrix4 SkeletonAttachPoint::GetLocalMatrix() const` | Transform attach pointa względem węzła-rodzica: `translate(RelativeLocation) * rotate(RelativeRotation)` (bez skali). Złóż z globalną macierzą rodzica (`Skeleton::AttachPoints` trzyma je po nazwie), żeby dostać pozycję w świecie. |
| `bool SkeletalMeshComponent::TryGetAttachPointWorldMatrix(const String& name, Matrix4& out)` | Pełna ramka świata attach pointa (`componentWorld * parentNodeGlobal * attachPointLocal`), liczona z **pozy z ostatniego builda snapshotu** — więc śledzi animację i live posing za darmo. `false`, gdy brakuje mesha/attach pointa/rodzica albo snapshot jeszcze nie poszedł. Bierz to zamiast pary `GetAttachPointLocationInWorld`/`GetAttachPointRotationInWorld`, gdy potrzebujesz całej bazy (np. doczepienie obiektu). |
| `Vec3 SkeletalMeshComponent::WorldLocationToNodeSpace(String nodeName, Vec3 worldLocation)` (`PLU_FUNCTION(PyExport)`) | Re-expresses a world-space location in the posed frame of skeleton node `nodeName`, using the world matrix **from the last pose build** (`CachedPoseWorldMatrix`), not the live one — inside `OnPreEvaluateAnimGraph` everything derived from poses is one frame stale, and the matching epoch makes that staleness cancel for anything riding the node rigidly. Built for feeding Bone-space graph goals (`EIKGoalSpace::Bone`); returns the input unchanged when the mesh/node/pose is missing. |
| `Vec3 SkeletalMeshComponent::WorldRotationToNodeSpace(String nodeName, Vec3 worldRotationDegrees)` (`PLU_FUNCTION(PyExport)`) | Rotation variant of the above; euler degrees both ways. |

### Płaska poza — `SkeletonPoseLayout`

Drzewo `RootNode` zostaje **formą źródłową** (import, serializacja, panele edytora chodzą po nim). `SkeletonPoseLayout` to jego **pochodna** forma adresowana indeksami, po której liczy się pozę co klatkę — bez hashowania nazw, bez `dynamic_cast`, bez rekursji.

Węzły w **DFS pre-order**, więc `ParentIndex[i] < i` — transformy globalne wychodzą jednym przelotem do przodu (rodzic zawsze gotowy przed dzieckiem). `BoneSlot` numeruje kości w tej samej kolejności co `CreateBonePalette`, więc **jest zgodny z `SkeletalVertex::BoneIndices`** — przy zmianie jednego trzeba ruszyć drugie.

Wszystkie pola to POD (żadnych `TOwningPointer`/`TUsePointer`), więc **zbudowany layout wolno czytać z worker threadów**; samo budowanie jest main-only.

| Funkcja / pole | Opis |
|---|---|
| `const SkeletonPoseLayout& Skeleton::GetPoseLayout() const` | Płaski widok szkieletu, budowany leniwie przy pierwszym użyciu i cache'owany na asset (zależy tylko od hierarchii, więc **współdzielony przez wszystkie instancje**). Pusty layout, gdy `RootNode` == null. |
| `void Skeleton::InvalidatePoseLayout() const` | Zrzuca cache. Potrzebne **tylko** kodowi, który edytuje `RootNode`/`Children` w miejscu po użyciu szkieletu (ścieżki importu). |
| `Int32 SkeletonPoseLayout::FindIndex(const String& nodeName) const` | Nazwa → indeks węzła, `-1` gdy nie ma. Do kroków bindujących (attach pointy, override'y pozy, tracki), które rozwiązują nazwę **raz** i dalej jadą na indeksie. |
| `void SkeletonPoseLayout::MakeBindPose(Pose& out) const` | Kopia bind pose w rozmiarze layoutu. Punkt startowy dla grafu, który nadpisuje tylko część kości. |
| `void SkeletonPoseLayout::ComposeGlobals(const Pose& local, Pose& outGlobal) const` | Poza lokalna (parent-space) → poza w przestrzeni szkieletu, jednym przelotem do przodu. `out` **nie może** aliasować `local`. Krótsza poza wejściowa dopełniana bind pose. |
| `void SkeletonPoseLayout::BuildBonePalette(const Pose& global, DynamicArray<std::pair<Matrix4,Matrix4>>& out) const` | Poza globalna → pary `(OffsetMatrix, globalMatrix)` pod shader, tylko kości, w kolejności `CreateBonePalette`. **Jedyne miejsce, gdzie transformy stają się macierzami — trzymać je na końcu łańcucha.** |
| `void SkeletonPoseLayout::BuildSubtreeMask(Int32 rootIndex, float insideWeight, float outsideWeight, DynamicArray<float>& outWeights) const` | Per-node blend weights for layered/masked blending: `rootIndex` and its whole subtree get `insideWeight`, every other node gets `outsideWeight`. One forward pass (DFS pre-order). `rootIndex < 0` → the whole mask is `outsideWeight`. Feeds `BlendPosesMasked` — used by `AnimLayeredBlendPerBoneNode`. |
| `ParentIndex[i]` / `BoneSlot[i]` | Indeks rodzica (`-1` = root) / slot w palecie skinningu (`-1` = węzeł nie-kość). |
| `LocalMatrix[i]` / `OffsetMatrix[i]` | Bind-pose local jak zaimportowany / inverse bind (identity tam, gdzie `BoneSlot < 0`). |
| `LocalBindTransform[i]` | `LocalMatrix[i]` zdekomponowany raz przy budowie — bind pose w formie, w której pracuje reszta pipeline'u. Fallback dla węzłów, których nie napędza żaden track. |
| `NodeName[i]` / `NameToIndex` | Nazwy do diagnostyki i bindowania — **nie tykać w pętli per-klatka**. |
| `Pose SkeletalMeshComponent::PosedGlobalTransforms` | Poza w przestrzeni szkieletu (root-relative) per **węzeł**, indeksowana indeksem z `SkeletonPoseLayout` (`CachedBonePalette` to wersja tylko-kości, macierzowa, pod shader). Producent: `RenderSnapshotBuilder`. Pusta do pierwszej ewaluacji; przeżywa trafienie w cache pozy. |

### Bone picker — `PluEngine/Animation/BoneRef.h` (`namespace Plu`)

`struct PLU_API BoneRef { String Name; }` — a reference to a skeleton node *by name*, same pattern as
`CollisionProfileRef`: a distinct type (not a bare `String`) so the editor renders a bone-hierarchy
dropdown via `TypeSerializer<BoneRef>` instead of a plain text field. Serializes as just the name.
Empty `Name` means "unset" — nodes using it (e.g. `AnimLayeredBlendPerBoneNode`, `AnimTransformBoneNode`)
treat `SkeletonPoseLayout::FindIndex(Bone.Name) < 0` as "nothing to do".

| Funkcja | Opis |
|---|---|
| `bool BoneRefEditorControl(void* value, const String& name)` | Editor-only (`PLU_ENGINE_EDITOR_BUILD`) combo widget: empty filter shows the skeleton hierarchy as an indented tree (`TreeNodeEx`, built once per popup open from `SkeletonPoseLayout::ParentIndex`); typing a filter switches to a flat case-insensitive `Selectable` list. |
| `void SetBonePickerSkeleton(Skeleton*)` / `Skeleton* GetBonePickerSkeleton()` | The skeleton bone pickers list nodes from. `BoneRef`'s `TypeSerializer::EditorControl` gets a bare `void*` and has no way to reach its owning node/asset, so the details panel sets this for the duration of drawing a node's properties (same trick as `ActiveCollisionConfig()` for `CollisionProfileRef`). Null → picker renders disabled. See `AnimationGraphDetailsPanel::OnUpdate`. |

## BoneTransform / Pose — `PluEngine/Animation/BoneTransform.h` (`namespace Plu`)

**Waluta całego pipeline'u animacji.** Klucze animacji są autorowane jako `Vec3`/`Quaternion`, każdy node grafu blenduje w tej formie, a konwersja do `Matrix4` następuje **dokładnie raz**, na samym końcu (`SkeletonPoseLayout::BuildBonePalette`). Macierze są i droższe w składaniu, i nie da się ich sensownie interpolować — nie wprowadzaj ich wcześniej.

`Rotation` to **kwaternion**, a nie Euler w stopniach jak `Vec3` rotacje w reszcie silnika (`GetForwardVector`, rotacja `GameObject`) — unikanie interpolacji kątów Eulera jest powodem istnienia tego typu.

`Pose` = `DynamicArray<BoneTransform>`, indeksowana indeksem węzła z `SkeletonPoseLayout` (wszystkie węzły, nie tylko kości). Lokalna albo w przestrzeni szkieletu — zależnie od tego, co ją wyprodukowało.

| Funkcja | Opis |
|---|---|
| `Matrix4 BoneTransform::ToMatrix() const` | `translate(Location) * mat4_cast(Rotation) * scale(Scale)`, bez budowania trzech macierzy po drodze. |
| `static BoneTransform BoneTransform::FromMatrix(const Matrix4&)` | Rozkład na T/R/S. **Shear jest gubiony** (nie da się go wyrazić) — dokładne dla bind pose i kluczy animacji, stratne dla macierzy ze skosem. Lustrzane odbicie (ujemny wyznacznik) ląduje w `Scale.x`, żeby `Rotation` została prawdziwą rotacją. |
| `BoneTransform BoneTransform::Compose(const BoneTransform& child) const` | Składanie hierarchii: `this` = rodzic, wynik = `parent.ToMatrix() * child.ToMatrix()` bez macierzy. **Uwaga:** niejednorodna skala rodzica + obrócone dziecko dają shear, którego T/R/S nie wyrazi → wynik przybliżony (to samo ograniczenie ma `FTransform` w UE). Jednorodna skala zawsze dokładna. |
| `BoneTransform BoneTransform::Inverse() const` | Transform odwrotny. Niezdefiniowany przy zerowej składowej `Scale`. |
| `Vec3 BoneTransform::TransformPoint(const Vec3&) const` | Punkt przez transform (skala → rotacja → translacja). |
| `void BoneTransform::NormalizeRotation()` | Renormalizuje `Rotation`. Długie łańcuchy blendów kumulują dryf — warto wołać, zanim poza opuści graf. |
| `BoneTransform BlendTransforms(const BoneTransform& a, const BoneTransform& b, float alpha)` | `alpha` 0 → `a`, 1 → `b`. Lokacja i skala lerp, rotacja **slerp najkrótszym łukiem**. |
| `BoneTransform BlendTransformsAdditive(const BoneTransform& base, const BoneTransform& additive, float alpha)` | Delta addytywna na wierzchu `base` (rotacja składana, lokacja i skala dodawane), skalowana `alpha`. Pod warstwy addytywne — aim offset, przechył, odrzut. |
| `void BlendPoses(const Pose& a, const Pose& b, float alpha, Pose& out)` | Wersja na całą pozę. `out` może aliasować `a`/`b`. Różne rozmiary → clamp do krótszego (degradacja zamiast asercji). |
| `void BlendPosesAdditive(const Pose& base, const Pose& additive, float alpha, Pose& out)` | Wersja addytywna na całą pozę. |
| `void BlendPosesMasked(const Pose& a, const Pose& b, const DynamicArray<float>& boneWeights, float defaultAlpha, Pose& out)` | **Maski kości / blending warstwowy** (np. górna połowa z jednej animacji, nogi z drugiej). `boneWeights[i]` = alpha dla węzła `i`; węzły poza zakresem tablicy dostają `defaultAlpha`. |

## Animacje szkieletowe — `PluEngine/AssetTypes/Animation/SkeletalAnimation.h` (`namespace Plu`, metody `AnimationTrack`)

Track trzyma klucze per kanał (`LocationKeys`/`RotationKeys`/`ScaleKeys`), **posortowane rosnąco po `Timestamp`** (ticki Assimpa). Samplery robią binary search + interpolację między sąsiednimi kluczami i clampują poza zakresem. Kanał może być pusty (FBX pivot-split) — wtedy zwracany jest `fallback` (domyślnie komponent identity).

| Funkcja | Opis |
|---|---|
| `Vec3 AnimationTrack::GetLocationAtTime(double timeTicks, const Vec3& fallback = Vec3(0)) const` | Lokacja w czasie (ticki), lerp między kluczami. |
| `Quaternion AnimationTrack::GetRotationAtTime(double timeTicks, const Quaternion& fallback = identity) const` | Rotacja w czasie, **slerp** + normalizacja. |
| `Vec3 AnimationTrack::GetScaleAtTime(double timeTicks, const Vec3& fallback = Vec3(1)) const` | Skala w czasie, lerp między kluczami. |
| `void AnimationTrack::SortKeys()` | Sortuje wszystkie trzy tablice kluczy po `Timestamp` — wołać po ręcznym wypełnieniu tablic (samplery tego wymagają). |
| `const DynamicArray<const AnimationTrack*>& Animation::GetTrackBinding(const Skeleton&) const` | Tracki rozwiązane po indeksach węzłów z `SkeletonPoseLayout`: `[i]` = track napędzający węzeł `i`, `nullptr` gdy animacja go nie rusza. **Bierz to zamiast `Tracks.Find(nazwa)` w pętli per-klatka.** Budowane raz, cache'owane na jeden szkielet naraz, współdzielone przez wszystkie komponenty grające tę animację. Main-only. |
| `void Animation::InvalidateTrackBinding() const` | Zrzuca binding. Dodanie/usunięcie tracka wykrywa się samo (po `Tracks.Size()`), ale **edycja istniejącego tracka w miejscu już nie** — wtedy zawołać ręcznie (tak samo jak `CachedPoseValid` na komponencie). |

## Node graph (reużywalny) — `PluEngine/NodeGraph/` (`namespace Plu`)

Generyczny szkielet grafu nodeów oparty na refleksji (patrz `project_nodegraph_system` w pamięci). Node = zreflektowana klasa polimorficzna (`PLU_STRUCT`, **nie** EngineObject). Domena (np. animacja) dziedziczy `GraphNode`/`NodeGraph`. **Nody i linki NIE są `PLU_PROPERTY`** — generyczny serializer tablicy gubi podtyp; zamiast tego jedzie `NodeGraphSerializer`.

| Funkcja / typ | Opis |
|---|---|
| `struct NodePin { String Name; EPinDirection Direction; EPinCategory Category; String TypeId; }` | Pin runtime (budowany, nie serializowany). `Flow` = drut domenowy (TypeId np. `"Pose"`), `Data` = wartość (TypeId = nazwa typu z refleksji, np. `"float"`). |
| `static bool NodePin::CanConnect(a, b)` | Reguła łączenia: przeciwne `Direction` ∧ ta sama `Category` ∧ ten sam `TypeId`. |
| `struct NodeLink { PluUUID FromNode; String FromPin; PluUUID ToNode; String ToPin; }` | Łącze trwałe po tożsamości (Uuid+nazwa pinu), nie po ephemeral id edytora. |
| `GraphNode` (`PLU_STRUCT(Abstract)`) | Baza node'a: `PluUUID Uuid`, `InputPins`/`OutputPins`, `virtual void BuildPins()`, `virtual String GetDisplayName()`, `void BuildDataPinsFromReflection()` (dodaje Data-piny z `PLU_PROPERTY` typów: float/double/bool/int/Int*/UInt*/Vec2-4), `NodePin* FindPin(name, dir)`. Tu też siedzi kontrakt data-pinów (`EvaluateDataOutput` / `ReadDataPin<T>`, niżej) — celowo na warstwie generycznej, nie na bazie domenowej, żeby node'y wartościowe działały w każdym grafie. |
| `NodeGraph : IAssetData` (`PLU_STRUCT`) | Właściciel: `DynamicArray<TOwningPointer<GraphNode>> Nodes` + `DynamicArray<NodeLink> Links`. API: `AddNode(TypeInfo*)`, `RemoveNode(uuid)`, `Connect(fromNode,fromPin,toNode,toPin)` (waliduje + 1 źródło na input), `Disconnect(link)`, `FindNode(uuid)`, `GetLinkSource(toNode,toPin)` (węzeł zasilający dany pin wejściowy, `nullptr` gdy odłączony — baza pod traversal), `FindInputLink(toNode,toPin)` (samo łącze, nie tylko węzeł-źródło — potrzebne, gdy trzeba też nazwy pinu źródłowego, np. odczyt data-pinów; `GetLinkSource` jedzie teraz przez to), `RebuildAllPins()`, `PruneInvalidLinks()`, `virtual TypeInfo* GetNodeBaseType()` (rodzina domenowa), `virtual bool AcceptsNodeType(TypeInfo*)` (paleta + `AddNode`). |
| `NodeGraphSerializer::Save(NodeGraph&, JSON&)` / `Load(dc, NodeGraph&, JSON&)` | Polimorficzny zapis/odczyt nodeów (`typeName`+`fields` przez `TypeSerializer<TypeInfo*>`) + linków. Wołać z loadera assetu (patrz `AnimationGraphAssetLoader`). |

Edytorowa warstwa canvasu (reużywalna, editor-only): `Editor/NodeGraph/` — `NodeGraphEditor::Draw(graph, onModified)` (rysowanie/łączenie/usuwanie/paleta/selekcja/layout), `NodeViewRegistry` + `INodeView`/`DefaultNodeView` (custom rysowanie per typ node'a). Pozycje nodeów = sidecar `<asset>.layout.json`, poza runtime assetem.

Paleta „Add Node" (`NodeGraphEditor::DrawAddNodeMenu`) buduje się z refleksji: każdy nieabstrakcyjny typ, który przechodzi `NodeGraph::AcceptsNodeType` (minus `SetPaletteTypeFilter`). Wpisy są sortowane i grupowane w podmenu po kategorii, plus filtr tekstowy na górze (przy aktywnym filtrze — jedna płaska lista `Kategoria > Etykieta`). **Kategoria wynika z hierarchii typów** — tabela `kPaletteCategories` w `NodeGraphEditor.cpp` mapuje nazwę bazy (`MathGraphNode` → „Math", `AnimGraphNode` → „Animation", …) i szuka jej w całym łańcuchu `BaseType`, więc pośrednie bazy (`MathBinaryNode`) nie psują grupowania. Nowa rodzina node'ów = nowa baza + jeden wiersz w tej tabeli. Etykieta = `TypeName` bez prefiksu kategorii i sufiksu `Node`, rozbite po CamelCase (`ConvertIntToFloatNode` → „Int To Float").

### Data-piny i node'y wartościowe (`PluEngine/NodeGraph/GraphValue.h`, `Nodes/`)

Warstwa liczenia wartości w grafie — domenowo neutralna, więc te same node'y działają w AnimGraphie i w każdym przyszłym grafie (`NodeGraph::AcceptsNodeType` przepuszcza `DataGraphNode` zawsze, obok własnej rodziny domenowej). **Piny nie mają żadnych niejawnych konwersji** (`NodePin::CanConnect` to porównanie stringów) — przejście między typami tylko przez node'y Convert.

| Funkcja / typ | Opis |
|---|---|
| `struct GraphEvalContext { NodeGraph* Graph; DynamicArray<PluUUID> DataEvalStack; virtual ~ }` | Baza kontekstu ewaluacji (domenowe konteksty, np. `AnimEvalContext`, dziedziczą ją). Polimorficzna, żeby node domenowy mógł zejść `dynamic_cast`-em po swoje dodatki (tak `AnimVariableNode` sięga po `Instance`). `DataEvalStack` = stos anty-cyklowy, obsługiwany w całości przez `ReadDataPin`. |
| `virtual bool GraphNode::EvaluateDataOutput(GraphEvalContext&, pinName, typeId, void* outValue)` | Wystawia wartość swojego data-outputu (zapis do `outValue`, storage typu `typeId`); zwraca czy się udało. Domyślnie `false`. Nadpisuje każdy node produkujący wartość. |
| `template<T> T GraphNode::ReadDataPin(GraphEvalContext&, pinName, const T& fallback) const` (protected) | Odczyt data-inputu: po linku do źródła, `EvaluateDataOutput` z `TypeId` **własnego** pinu. Pin odłączony / brak źródła / niezgodny typ / **wykryty cykl** → `fallback`. **Wzorzec każdego node'a z data-pinem: czytaj przez `ReadDataPin`, własne pole jako fallback** (`AnimBlendNode::Alpha`, `MathAddNode::A`). Ewaluacja jest pull-based, bez memoizacji — diament liczy oba ramiona dwa razy. |
| `enum class EGraphValueType { Float, Int, Bool, Vec3 }` + `struct GraphValue` | Wartość o typie wybieranym w runtime: `Type` + pola `Float/Int/Bool/Vector`, `PinTypeId(type)` (`"float"`/`"int"`/`"bool"`/`"Vec3"` — te same nazwy co typy zmiennych AnimGrafu, więc kolory pinów i wpinanie zmiennych działają same) i `CopyTo(dst, typeId)`. **Nie jest zreflektowany** — jedzie ręcznym `TypeSerializer<GraphValue>` (jak `glm::vec3`), który w details panelu rysuje **tylko** widget aktywnego typu. |
| `DataGraphNode` (`PLU_STRUCT(Abstract)`) | Baza node'a wartościowego: `PLU_PROPERTY EGraphValueType ValueType` (wildcard — jeden typ node'a zamiast jednego na (operacja, typ)), `SupportedTypes()` (maska `TypeBit(...)`, `ClampValueType()` przyciąga `ValueType` do wspieranego), `AddValueInput/Output(name, type)`, `EvalBinary<Op>`/`EvalUnary<Op>` (dispatch po `ValueType` na funktory z `Nodes/GraphNodeMathOps.h`), `IsOutput(pin, typeId, expectedPin, expectedType)`, `OperationName()` (tytuł node'a = operacja + aktywny typ, np. „Add (Vec3)"). Kategorie: `MathGraphNode`, `VectorGraphNode`, `LogicGraphNode`, `ConvertGraphNode`. |
| `Nodes/MathNodes.h` | `MathAdd/Subtract/Multiply/Divide/Min/Max` (baza `MathBinaryNode`, piny `A`/`B`→`Result`), `MathAbs/Negate` (`MathUnaryNode`), `MathClamp` (`Value`/`Min`/`Max`), `MathLerp` (`A`/`B` + zawsze-float `Alpha`), `MathConstant`. Wildcard Float/Int/Vec3 (Constant + Select także Bool, Lerp bez Int). Vec3 `*`/`/` są **po składowych**; dzielenie przez zero daje 0 zamiast inf/NaN. |
| `Nodes/VectorNodes.h` | Stałotypowe: `VectorScale` (Vec3×float), `VectorLength`, `VectorNormalize` (zero → zero, nie NaN), `VectorDot`, `VectorCross`, `VectorDistance`, `VectorMake` (X/Y/Z→Vec3), `VectorBreak` (Vec3→X/Y/Z, jedyny node z wieloma data-outputami). |
| `Nodes/LogicNodes.h` | `LogicAnd/Or/Not` (bool), `LogicGreater/Less/Equal` (operandy wildcard Float/Int, wyjście bool; równość floatów z epsilonem 1e-4), `LogicSelect` (`Condition ? A : B`, dowolny typ — **bez short-circuitu**, oba ramiona się liczą). |
| `Nodes/ConvertNodes.h` | `ConvertIntToFloat`, `ConvertFloatToInt` (obcina do zera). Jedyna droga między typami pinów. |

**Dodanie nowego node'a wartościowego:** `PLU_STRUCT()` + dziedziczenie po odpowiedniej kategorii, `BuildPins()` (piny) i `EvaluateDataOutput` — nic więcej (brak fabryki, brak CMake, brak rejestracji w palecie; generator refleksji ogarnia resztę). Kilka node'ów w jednym nagłówku jest OK — generator robi jeden `*.generated.h` na **plik**. **Uwaga:** własność zmieniająca topologię pinów wymaga `RebuildAllPins()` + `PruneInvalidLinks()` po edycji (robi to details panel AnimGrafu).

### AnimGraph — runtime ewaluacji (`PluEngine/AssetTypes/AnimationGraph/`)

Traversal + sampling/blend, zaimplementowane 2026-07-21 (wcześniej stuby). Bezstanowe: każde wywołanie liczy pozę od zera z `AnimEvalContext::TimeSeconds`, nic nie jest cache'owane na węźle/grafie (state machines / per-instance state = przyszłość).

| Funkcja / typ | Opis |
|---|---|
| `struct AnimEvalContext : GraphEvalContext { float TimeSeconds; bool Loop; TUsePointer<Skeleton> TargetSkeleton; AnimGraphInstance* Instance; Matrix4 ComponentToWorld; }` | Nie-reflected, budowany na nowo per wywołanie ewaluacji przez wołającego (`RenderSnapshotBuilder`, patrz niżej). `Graph` (z bazy) ustawia `AnimationGraph::Evaluate` — nie wypełniać ręcznie. `TargetSkeleton` może być pusty (podgląd grafu bez szkieletu) — nody wtedy zwracają pustą pozę. `Instance` = per-user wartości zmiennych (patrz sekcja "AnimGraph — instancje per użytkownik" niżej); `nullptr` = nody czytają wartości domyślne z assetu (podgląd w edytorze bez PIE). `ComponentToWorld` = world matrix of the driving component; used only by World-space nodes (`AnimTransformBoneNode`), identity when unknown (e.g. editor graph preview with no bound component — World then degenerates to Component space). |
| `Pose AnimGraphNode::EvaluateInputPose(AnimEvalContext&, const String& pinName) const` (protected) | Idzie po linku wpiętym w `pinName` do węzła źródłowego i woła jego `Evaluate`. Pin odłączony / źródło nie jest `AnimGraphNode`: fallback = bind pose z `TargetSkeleton->GetPoseLayout()` (albo pusta poza, gdy brak szkieletu). Tego używa każdy konkretny node zamiast ręcznego `GetLinkSource`+`dynamic_cast`. |
| data-piny | `ReadDataPin<T>` / `EvaluateDataOutput` mieszkają na `GraphNode` (sekcja „Data-piny i node'y wartościowe" wyżej), nie na `AnimGraphNode` — `AnimBlendNode::Alpha` czyta dokładnie tak samo jak node matematyczny. `AnimVariableNode` nadpisuje `EvaluateDataOutput`: `dynamic_cast<AnimEvalContext*>` po `Instance` (wartość żywa), fallback na `Variable` assetu. |
| `Pose AnimationGraph::Evaluate(AnimEvalContext&)` | Punkt wejścia: liniowo szuka `AnimOutputPoseNode` w `Nodes`, ustawia `context.Graph = this`, zwraca jego `Evaluate` (rekursywnie ciągnie graf w górę). Pusta poza gdy brak output node'a. |
| `AnimSampleNode::Evaluate` | Sekundy z kontekstu → ticki (`* Animation::FramesPerSecond`), `fmod`/clamp wg `context.Loop`, potem `Animation::GetTrackBinding(*skeleton)` indeksowany po `SkeletonPoseLayout` — dokładnie wzorzec z `RenderSnapshotBuilder`. Start od `layout.MakeBindPose()`, nadpisywane per-node tylko gdzie jest track. |
| `AnimBlendNode::Evaluate` | `EvaluateInputPose` na pinach `"A"`/`"B"`, `ReadDataPin<float>(context, "Alpha", Alpha)` dla współczynnika, `BlendPoses(a, b, alpha, result)`. |
| `AnimBlendByBoolNode::Evaluate` | Piny pozy `"True"`/`"False"` → `"Result"`, warunek z `ReadDataPin<bool>(context, "Condition", Condition)` (bool data-pin z `BuildDataPinsFromReflection`). **Liczy tylko wybraną gałąź** (przegrana może być całym poddrzewem animacji). Przełączenie jest natychmiastowe — cross-fade w czasie (blend time z UE) wymagałby stanu per instancja, a ewaluacja grafu jest bezstanowa; płynne przejście = `AnimBlendNode` ze sterowaną `Alpha`. |
| `AnimOutputPoseNode::Evaluate` | `return EvaluateInputPose(context, "Pose")`. |
| `AnimLayeredBlendPerBoneNode::Evaluate` | Splits the skeleton at `Bone` (a `BoneRef`): everything OUTSIDE its subtree comes from pose `"A"`, `Bone` and its whole subtree from `"B"`, weighted by `ReadDataPin<float>(context, "Alpha", Alpha)`. Builds the per-node weights via `SkeletonPoseLayout::BuildSubtreeMask` and blends with `BlendPosesMasked`. `Alpha` is a hard per-subtree weight (no falloff up the hierarchy) — the cut at the chosen joint is sharp by design. `alpha <= 0`, no `TargetSkeleton`, or `Bone` not found on the skeleton → returns `"A"` unevaluated (`"B"` can be a whole animation sub-tree, same reasoning as `AnimBlendByBoolNode`'s untaken branch). |
| `AnimTransformBoneNode::Evaluate` | Modifies one bone (`BoneRef Bone`) in-place: `Translation`/`Rotation`/`Scale` (`Vec3`, Rotation in degrees), each with its own `EBoneModifyMode` (`Ignore`/`Add`/`Replace`), authored in `EBoneTransformSpace` (`Local`/`Component`/`World`). `World` composes through `AnimEvalContext::ComponentToWorld`; `Component` composes through `SkeletonPoseLayout::ComposeGlobals`. Result is converted back to Local (parent-space, undoing the same composition) before being blended into the bone's local transform by `Alpha` — descendants are not touched directly, they ride along once `ComposeGlobals` runs downstream. All three modes `Ignore`, `alpha <= 0`, no `TargetSkeleton`, or `Bone` not found → returns the input pose unchanged. |
| `AnimationGraphVariableFactory::RegisterBuiltInTypes()` | Rejestruje wbudowane typy zmiennych (Integer/Float/Boolean/String/Vec3). Wołane raz z `Application::EngineInit()` — **Editor i Runtime dzielą tę samą fabrykę** (wcześniej robił to tylko edytor, więc Runtime miał pustą fabrykę i `AnimationGraphAssetLoader` gubił każdą zmienną przy wczytaniu grafu). |

**Podpięte do renderowania (2026-07-21):** `SkeletalMeshComponent` ma `PLU_PROPERTY() TUsePointer<AnimationGraph> AnimGraph` obok istniejącego `AnimationToShow` — **graf ma priorytet, gdy przypisany**, ale surowa animacja NIE jest kasowana ani ignorowana na stałe: odpięcie grafu (`AnimGraph = nullptr`) wraca od razu na `AnimationToShow`. Osobny licznik czasu `float GraphTimeSeconds` (runtime-only, jak `AnimationTimeTicks`) — graf nie ma jednego wspólnego FPS jak pojedyncza animacja, więc `AnimEvalContext::TimeSeconds` jedzie osobno; `OnUpdate` posuwa oba liczniki niezależnie, gdy `IsPlaying`, i przepisuje `GraphTimeSeconds` do `instance->TimeSeconds`. `RenderSnapshotBuilder.cpp` (~linia 436, `"Skeletal Mesh Calculations"`): gałąź `if (animGraph) { ...AnimationGraph::Evaluate → lokalna poza → BoneLocalOverrides → SkeletonPoseLayout::ComposeGlobals... } else { /* stara pętla sample-and-compose dla AnimationToShow */ }`, obie kończą się w `layout.BuildBonePalette`. Cache pozy (`CachedPoseAnimUuid`/`CachedPoseTicks`) klucz teraz źródło-agnostyczny (`poseSourceUuid`/`poseTimeKey` = uuid+czas grafu **albo** animacji, którykolwiek aktywny) **plus** `CachedPoseGraphValueRevision` (patrz niżej — `SetFloat` przy zatrzymanym czasie nie zmieniałby nic innego w kluczu) **plus** `CachedPoseWorldMatrix` (`Matrix4`, compared against `worldComponent->GetWorldMatrix()`) — added for World-space graph nodes (`AnimTransformBoneNode`), which fold the component's world matrix into the local-space pose itself, so unlike everything else in this key the pose stops being independent of the component's transform. Costs 16 float comparisons per component per frame and a cache miss on every move even without a world-space node.

### AnimGraph — instancje per użytkownik (`PluEngine/Animation/AnimGraphVariableStore.h`, `AnimGraphInstance.h`)

Asset (`AnimationGraph::Variables`) trzyma tylko **definicje + wartości domyślne**. Żywe, per-komponentowe wartości (żeby dwie postacie z tym samym grafem mogły mieć różne `Speed`) żyją w `AnimGraphInstance`, jednej na `SkeletalMeshComponent`.

| Funkcja / typ | Opis |
|---|---|
| `TOwningPointer<IAnimationGraphVariable> IAnimationGraphVariable::Clone() const` | Głęboka kopia (Name/TypeName/PinTypeId + wartość) do świeżo skonstruowanej zmiennej tego samego konkretnego typu. Zaimplementowane raz w `AnimationGraphVariable<T>`. |
| `bool IAnimationGraphVariable::CopyValueTo(void* dst, const String& expectedPinTypeId) const` | Kopiuje wartość do `dst`, gdy `expectedPinTypeId == PinTypeId` (przez przypisanie `T`, nie `memcpy` — działa też dla `String`). Ścieżka odczytu data-pinów (`ReadDataPin`/`EvaluateDataOutput`). |
| `struct AnimGraphVariableStore` | Czysty magazyn wartości (nazwa → sklonowana zmienna), nic nie wie o nodach/grafie/assecie. `RebuildFrom(defaults)` (od zera), `MergeFrom(defaults)` (zachowuje wartości dla zmiennych o tej samej nazwie **i** `TypeName` — edycja listy zmiennych w PIE nie kasuje żywego stanu), `Find(name)`, `GetAll()` (cała lista, w kolejności), `template<T> bool Set(name, value)` / `template<T> bool TryGet(name, outValue)` (type-safe przez `dynamic_cast<AnimationGraphVariable<T>*>`), `GetValueRevision()` / `MarkValueChanged()` (bumpowana przez `Set`, `RebuildFrom`, `MergeFrom` gdy coś się realnie zmieniło; klucz cache'a pozy). |
| `struct AnimGraphInstance` (`PLU_STRUCT(PyExport)`) | Żywe wartości jednego "użytkownika" grafu. `BindTo(graph)` — (re)klonuje domyślne zmienne; tani no-op gdy uuid+`VariablesRevision` bez zmian, `MergeFrom` gdy ten sam graf zmienił listę zmiennych, `RebuildFrom` gdy graf się zmienił. `GetVariables()`, `GetGraph()`, `template<T> Set/TryGet` (C++). Python: `SetFloat/GetFloat`, `SetInt/GetInt`, `SetBool/GetBool`, `SetString/GetString`, `SetVec3/GetVec3`, `HasVariable(name)`, `GetVariableNames()`. Pole `float TimeSeconds` — zegar ewaluacji per instancja (docelowo tu wjedzie stan maszyn stanów). Editor-only: `String DebugName`, `static DynamicArray<AnimGraphInstance*>& GetLiveInstances(graphUuid)` (rejestr żywych instancji per asset, do panelu Variables w PIE). |
| `AnimGraphInstance* SkeletalMeshComponent::EnsureAnimGraphInstance()` | Tworzy przy pierwszym użyciu, woła `BindTo(AnimGraph)` (tanie gdy nic się nie zmieniło), `nullptr` gdy brak `AnimGraph`. Wołane co klatkę z `RenderSnapshotBuilder` (nie tylko podczas `IsPlaying`) i z `OnUpdate`. |
| `AnimGraphInstance* SkeletalMeshComponent::GetAnimGraphInstance()` (`PLU_FUNCTION(PyExport)`) | Alias Pythona dla `EnsureAnimGraphInstance()` — `comp.GetAnimGraphInstance().SetFloat("Speed", 5.0)`. |
| `UInt32 SkeletalMeshComponent::CachedPoseGraphValueRevision` | Dodatkowy klucz cache'a pozy obok mesh/anim/ticks: `AnimGraphInstance::GetVariables().GetValueRevision()` w momencie ostatniego builda. Bez tego `SetFloat` przy zatrzymanym czasie (`IsPlaying=false`) nie zmieniałby pozy (cache hit na reszcie klucza). |

Edytor (`AnimationGraphVariablesPanel`/`AnimationGraphViewport`/`AnimationGraphDetailsPanel`): każda mutacja listy zmiennych (Add/Delete/rename) bumpuje `AnimationGraph::VariablesRevision` (nie-serializowane, nie `PLU_PROPERTY`) obok `PanelChangedAsset()`. W PIE, gdy graf ma żywe instancje, Variables panel pokazuje combo `Defaults` + `DebugName` każdej instancji (`AnimationGraphViewport::SetInspectedInstance`/`GetInspectedInstance`); wybór zmienia, przeciw czemu rozwiązuje się `AnimationGraphViewport::GetSelectedVariable()` (nazwa → `store.Find` albo `graph->FindVariable`). Details panel w trybie live: Name/Type read-only, wartość edytowalna, **bez `PanelChangedAsset()`** (patrz `Editor/CLAUDE.md`, "Czego NIE brudzić") — zamiast tego `store.MarkValueChanged()`.

## Stringi (engine) — `PluEngine/PluUtils.h` (`namespace Plu`)

| Funkcja | Opis |
|---|---|
| `String MakeStringForDisplay(String text)` | Rozbija `CamelCase` na słowa rozdzielone spacją (z cache'em); do labeli w UI. |
| `String PrepareCodeForDistribution(String code)` | Usuwa komentarze i nadmiarowe whitespace z kodu (minifikacja przed dystrybucją). |

## Konwersje Jolt ↔ GLM — `PluEngine/PluUtils.h` (`namespace Plu`)

`static` inline, do mostkowania matematyki Jolt Physics i glm:

| Funkcja | Opis |
|---|---|
| `JPH::RVec3 ToJPH(const Vec3&)` | `Vec3` → `JPH::RVec3`. |
| `Vec3 ToGLM(const JPH::RVec3&)` | `JPH::RVec3` → `Vec3`. |
| `JPH::Vec3 ToJPHVec3(const Vec3&)` | `Vec3` → `JPH::Vec3`. |
| `Vec3 ToGLMFromVec3(const JPH::Vec3&)` | `JPH::Vec3` → `Vec3`. |

---

## Renderer / cienie — `PluEngine/Renderer/RenderUtils.h` (`namespace Plu`)

Stałe kamery: `kCameraNearClip = 0.1f`, `kCameraFarClip = 100000.0f`. Zasięg cieni nie jest już globalną stałą — to ustawienie per światło (`DirectionalLight::ShadowDistance`, domyślnie 150 m).

| Funkcja | Opis |
|---|---|
| `DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)` | 8 narożników frustum w przestrzeni świata (alokuje). |
| `void GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view, Vec3 Out[8])` | Wariant bez alokacji — pisze do tablicy wywołującego. Używany na ścieżce kaskad (per klatka). |
| `Matrix4 GetCascadeProjectionMatrix(float fovY, float aspect, float near, float far)` | Projekcja perspektywiczna pod-frustum jednej kaskady. |
| `void ComputeCascadeSplits(const CascadeConfig&, float NearClip, DynamicArray<float>& Out)` | Podział `[NearClip, ShadowDistance]` na `CascadeCount` odległości (`SplitLambda`: 0=liniowy, 1=logarytmiczny). `Out` jest `Clear()`owane — capacity zostaje, więc w ustalonym stanie zero alokacji. |
| `Int32 ComputeAutoPcfTapCount(float PcfRadiusTexels)` | Liczba tapów PCF, która dokładnie pokrywa dysk o danym promieniu: `ceil(pi * r²)`, clamp do `[1, kMaxShadowPcfTaps]`. Jeden tap to sprzętowe 2x2, czyli uśrednia ~1 teksel², więc dysk o polu `pi*r²` tekseli potrzebuje tylu tapów — mniej daje obrączki na miękkiej krawędzi, więcej czyta drugi raz to samo. Używane przez `Renderer::ClampShadowSettings`, gdy `DirectionalLight::ShadowPcfAutoTaps` jest włączone. |
| `void ComputeCascadeMatrices(cameraView, fovY, aspect, nearClip, lightDir, config, splits, Out, perCascadeRes = nullptr)` | Macierze światła (proj*view) wszystkich kaskad CSM do `Out` (też bez alokacji). **Stabilność**: stała baza światła (`lookAt(-lightDir, 0, up)` — nic z kamery), promień sfery zaokrąglany w górę do 1/16 m, snap środka do siatki teksela **w tej bazie** (snap w bazie zależnej od kamery jest matematycznym no-opem). Bez marginesu near — pass cieni używa `GL_DEPTH_CLAMP` (pancaking). `perCascadeRes` nadpisuje `config.Resolution` per kaskada (legacy zestaw o mieszanych rozdzielczościach). |

`struct ShadowCascadeData { Matrix4 ViewProj; float SplitDistance; float TexelWorldSize; float Radius; }` —
`TexelWorldSize` (= `2*Radius/Resolution`) niesie normal-offset odbiorcy, `Radius` przelicza bias w metrach na `[0,1]` głębi kaskady.

`struct CascadeConfig { Int32 CascadeCount; float ShadowDistance; float SplitLambda; Int32 Resolution; }`.

`Plu::kMaxShadowCascades` (= 4) wymiaruje jednocześnie tablice w bloku GLSL `ShadowData` i tablicę
tekstur cieni — jedno miejsce, żaden `#define` nie jest synchronizowany ręcznie.
`Plu::kMaxShadowPcfTaps` (= 32) odpowiada `MAX_PCF_TAPS` w `PBR.frag` (limit pętli filtra PCF).

`struct ShadowDataGPU` — mirror std140 bloku `ShadowData` (**UBO binding 2**) z PBR.frag: macierze
kaskad, splity, rozmiary teksela, biasy, fade, blend, `CascadeCount`, flaga debug,
`InvShadowMapResolution`, `PcfTapCount`, `PcfRotateSamples`. Wypełniany i uploadowany
**bezwarunkowo co klatkę** przez `Renderer::UpdateShadowDataBuffer` (bez światła → `CascadeCount = 0`),
więc shadery nigdy nie czytają stanu z poprzedniej klatki. Offsety pilnowane `static_assert`ami —
przy zmianie struktury zmień **oba** miejsca (C++ i GLSL).

### Ostrość cieni kierunkowych

Filtr PCF w `PBR.frag` to **dysk Vogela** (`VogelDiskSample`, złoty kąt) o `pcfTapCount` próbkach
i promieniu `pcfRadiusTexels`, opcjonalnie obracany per piksel przez `InterleavedGradientNoise`
(`gl_FragCoord`). Każdy tap to sprzętowe porównanie `sampler2DArrayShadow`, czyli już 2x2 PCF.
`pcfRadiusTexels == 0` lub `pcfTapCount == 1` zwija filtr do jednego pobrania — najostrzejsza
krawędź, jaką mapa potrafi dać.

Liczba tapów **nie jest suwakiem jakości** — to budżet próbek, który musi nadążyć za promieniem.
Dlatego `DirectionalLight::ShadowPcfAutoTaps` (domyślnie **on**) wylicza ją z promienia przez
`ComputeAutoPcfTapCount` i `ShadowPcfTaps` jest wtedy ignorowane. Skala: promień 1,5 teksela →
8 tapów, 3,0 → 29, 4,0 i wyżej → limit 32. Stąd bierze się typowe „32 tapy wyglądają jak 8" —
przy małym promieniu ósemka pokrywa dysk w całości i nadmiar próbek liczy w kółko to samo.
Rozdzielczanie do konkretnej liczby dzieje się na CPU (`Renderer::ClampShadowSettings`), więc UBO
i statystyki pokazują liczbę faktycznie próbkowaną, nie tryb.

Dwie osie sterują wyglądem krawędzi i mylenie ich to najczęstszy błąd:

- **Ostrość** = szerokość półcienia → `ShadowPcfRadius` (mniej = ostrzej).
- **Pikselowatość** = teksel kaskady większy niż piksel ekranu → `ShadowResolution`
  (512…8192), `ShadowCascadeCount`, `ShadowDistance`, `ShadowSplitLambda`. Sam mniejszy promień
  PCF tego **nie naprawi** — odsłoni.

`ShadowPcfRotate` jest półśrodkiem między nimi: rozbija schodki na dither szerokości piksela, więc
mały promień wygląda ostro zamiast blokowo. Kosztem jest lekki szum na krawędzi (bez temporalnego
wygładzania nie ma go czym uśrednić) — wyłącz, jeśli w danej scenie przeszkadza bardziej niż schodki.

VRAM mapy cieni to `Resolution² × 4 B × CascadeCount` (D32F): 268 MB przy 4096/4, **1,07 GB** przy
8192/4 — 8192 łącz z mniejszym `ShadowDistance` albo mniejszą liczbą kaskad.

### Frustum culling — `PluEngine/Renderer/RenderUtils.h`

| Funkcja | Opis |
|---|---|
| `Frustum ExtractFrustumPlanes(const Matrix4& viewProj)` | 6 płaszczyzn frustum (Gribb-Hartmann, znormalizowane) z macierzy view*proj. `struct Frustum { Vec4 Planes[6]; }` — `(nx,ny,nz,d)`, wewnątrz gdy `dot(n,p)+d >= 0`. |
| `bool SphereInFrustum(const Frustum&, const Vec3& center, float radius)` | Test sfera-vs-frustum (6 testów płaszczyzna-punkt). |
| `bool SphereInFrustumNoNear(const Frustum&, const Vec3& center, float radius)` | To samo bez płaszczyzny near. **Tego** używa culling casterów cieni: pass cieni renderuje z `GL_DEPTH_CLAMP`, więc caster przed płaszczyzną near jest spłaszczany NA nią i dalej zasłania — test near wyciąłby dokładnie obiekty między światłem a sceną. |

### Static mesh: draw calls i bounding box — `PluEngine/AssetTypes/StaticMesh/StaticMesh.h`, `PluEngine/Physics/BoundingBox.h`

| Symbol | Opis |
|---|---|
| `void DrawStaticMesh(const StaticMesh*, RenderingManager*)` | Jeden `glDrawElements`. |
| `void DrawStaticMeshInstanced(const StaticMesh*, RenderingManager*, UInt32 instanceCount)` | Jeden `glDrawElementsInstanced` — `instanceCount` instancji naraz, dane per-instancja idą przez SSBO `InstanceMatrices` (indeks `gl_InstanceID` + uniform `instanceBaseIndex`, patrz `Renderer::RenderSnapshot` i `Renderer::RenderShadowPass`). Wywołuje `OnStaticMeshRender` **raz**, nie N razy (to flaga żywotności dla eviction, nie licznik populacji). Programy z blokiem `InstanceMatrices` (`BasicVertInstanced.vert`, `OnlyPositionInstanced.vert`) celowo **nie mają** `uniform mat4 model` — dla nich to jedyna poprawna ścieżka rysowania, niezależnie od liczby instancji. |
| `EngineAssets::OnlyPositionInstancedShader` | Shader głębi (SSBO `InstanceMatrices`) dla static meshy w mapach cieni (`Renderer::RenderShadowPass`) — silnikowy, **zawsze** instanced (nie opt-in per materiał jak główny pass, bo depth pass nie używa materiału sceny). Zastąpił dawny `OnlyPositionShader`, który wraz z `OnlyPosition.vert` został usunięty. |
| `BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh*)` | Chodzi po **każdym wierzchołku** — nigdy per klatka, cache'uj (patrz `StaticMeshComponent::MeshBoundingBoxComputed` / `InstancedStaticMeshComponent`). |
| `BoundingBox CreateBoundingBoxForSkeletalMesh(SkeletalMesh*)` | To samo dla skeletal mesha (bind pose). Cache'owane w `SkeletalMeshComponent::MeshBoundingBox` + `MeshBoundingBoxComputed`. Animacja wypycha wierzchołki poza te bounds — `RenderSnapshotBuilder` rozdmuchuje promień przed użyciem do cullingu. |
| `StaticMeshComponent::MeshBoundingBox` / `MeshBoundingBoxComputed` | Bounding box (local space) komponentu; `MeshBoundingBoxComputed` to twardy guard — liczony raz w `SetStaticMesh` (jeśli mesh już załadowany) albo leniwie w `RenderSnapshotBuilder`, gdy mesh dojedzie asynchronicznie. |

### Introspekcja shaderów — `PluEngine/Shaders/ShaderProgram.h`

| Symbol | Opis |
|---|---|
| `bool ShaderProgram::HasBoneMatricesBlock()` | Czy zlinkowany program deklaruje blok SSBO `BoneMatrices` (vertex skinning). GL query cache'owane per link (reset przy `UnloadProgram`/`LoadFromBinary`); wołać z **render threadu** po `IsLoaded()`. Renderer używa tego do warninga, gdy skeletal mesh dostaje materiał bez skinningu (mesh stałby w bind pose po cichu). |
| `bool ShaderProgram::HasInstanceDataBlock()` | Jak wyżej, dla bloku SSBO `InstanceMatrices` (instancing static meshy). Renderer używa tego do wyboru `DrawStaticMeshInstanced` vs fallback per-obiekt (opt-in: materiał na programie bez tego bloku renderuje się identycznie jak dziś). |
| `ShaderProgram::Set*Uniform(...)` | Settery uniformów: lokacja cache'owana per nazwa (jeden lookup `Find`), **no-op bez żadnego wywołania GL**, gdy uniform nie istnieje w programie (lokacja -1) — ustawianie uniformów globalnych na wszystkich programach jest tanie. `SetTextureUniform` przy braku uniformu **nie binduje** tekstury. |
| `static ShaderProgram::ResetBindCache()` | `ShaderProgram::Bind()` deduplikuje `glUseProgram` cache'em aktualnie zbindowanego programu (render thread only). Wołać po każdym miejscu, które binduje/kasuje program **poza** `ShaderProgram::Bind` (np. backend ImGui) — `Renderer::RenderSnapshot` robi to na starcie każdej klatki, `UnloadProgram` przy kasowaniu. |

**Punkty bindingu buforów (silnikowa konwencja, nie zmieniać bez powodu):**

| Binding | Blok | Kto binduje |
|---|---|---|
| SSBO `0` | `BoneMatrices` — palety skinningu **wszystkich** skeletal meshy klatki | `Renderer::UploadSkeletalPalettes`, raz na klatkę |
| SSBO `1` | `InstanceMatrices` — dane instancji static meshy | `Renderer::RenderSnapshot`, raz na klatkę |
| UBO `2` | `ShadowData` — parametry cieni kaskadowych | `Renderer::UpdateShadowDataBuffer`, **bezwarunkowo** co klatkę |
| SSBO `3` | `VisibleInstanceIndices` — indeksy casterów, które przeszły culling kaskad | `Renderer::CullShadowCasters` |

Tablica map cieni siedzi na slocie tekstury **15** (ostatnim gwarantowanym przez GL 4.5), a tekstury materiału startują od **0** (`SetSlotsUsed(0)`).

**Dlaczego nie slot 0:** `RenderFromMaterial` woła `SetTextureUniform` **tylko** dla samplerów, którym faktycznie przypisano teksturę — sampler bez tekstury zostaje na domyślnym slocie 0. Dwa samplery **różnych typów** (`sampler2D` materiału + `sampler2DArrayShadow`) na tym samym slocie w jednym programie to `INVALID_OPERATION` przy rysowaniu; NVIDIA to ignoruje, Mesa odrzuca draw i scena robi się czarna. Zmieniając slot cieni zmień **oba** miejsca: `Renderer::kShadowTextureUnit` i `layout(binding = ...)` w `PBR.frag`.

Wszystkie `BindBase` idą **po** ewentualnym `Resize` — `Resize` tworzy nowe ID bufora, a indeksowany punkt bindowania trzymałby skasowany. Palety skinningu liczy raz na klatkę `Renderer::BuildSkeletalPalettes` (płaski scratch + zakresy per obiekt), a `UploadSkeletalPalettes` wysyła je **jednym** uploadem; rysowanie adresuje swój zakres uniformem `paletteBaseIndex` (`BasicVertSkeletal.vert`, `OnlyPositionSkeletal.vert`). Analogicznie `instanceBaseIndex` — w passie głównym offset w `instances`, a w `OnlyPositionInstanced.vert` offset w `visibleIndices`.

**Uwaga:** każdy nowy luźny uniform sterowany przez silnik musi trafić do `engineOnlyUniforms` w `PythonTools/ShaderCodeParser.py` w tej samej zmianie — inaczej parser wciągnie go jako parametr materiału i `RenderFromMaterial` nadpisze go zserializowaną wartością w środku klatki (w pliku udokumentowane dwa takie błędy).

### Wrappery zasobów GL — `PluEngine/Renderer/`

| Symbol | Plik | Opis |
|---|---|---|
| `class Texture` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLTexture.h` | Tekstura 2D **lub tablica warstw**: `Create`/`CreateFromInfo`/`CreateDepth` (opcjonalne `Use16Bit` = D16 zamiast D32F), `CreateDepthArray(w, h, layers, Use16Bit)` → `GL_TEXTURE_2D_ARRAY` D32F, `Bind(unit)`, streaming mipów, `SaveTexture`. Move-only. Target trzymany w obiekcie (`GetTarget`/`IsArray`/`GetLayerCount`); operacje z natury 2D (streaming mipów, `SaveTexture`) mają `PLU_CORE_ASSERT` na target. |
| `class FrameBuffer` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLFrameBuffer.h` | FBO: `Create` (opcjonalne `Use16BitDepth` dla DepthOnly)/`CreateWithTexture`/`CreateDepthOnly`/`CreateWithDepthTextureLayer(array, layer, objMgr)`, `Bind`/`Resize`/`BlitTo`, `FrameBufferType` (Color/ColorDepth/DepthOnly/DepthStencil). Move-only. FBO warstwy **nie jest właścicielem** tablicy (kilka FBO celuje w tę samą teksturę) i **nie wspiera `Resize`** — właściciel przebudowuje całość. |
| `template<typename T> class UniformBuffer` | `Renderer/GLUniformBuffer.h` | Wrapper UBO na jeden blok POD `T` (std140). Header-only, move-only, jak `ShaderStorageBuffer`. API: `Create(init,usage)`, `Update(data)`, `Bind`/`BindBase(binding)`, `Destroy`. Zawartość widzą **wszystkie** programy naraz — dlatego parametry cieni ustawia się raz na klatkę, a nie per program. std140 jest ostrzejszy niż std430: `vec3`/`vec4`/`mat4` wyrównane do 16 B, elementy tablic **zawsze** dopadowane do 16 B — deklaruj wektory/macierze pierwsze, skalary na końcu i asertuj offsety. |
| `class SamplerObject` | `Renderer/GLSamplerObject.h` | Wrapper obiektu samplera GL (filtrowanie/wrap/porównanie **na jednostce teksturującej**, nie na teksturze). Header-only, move-only. API: `Create`, `SetFilter`, `SetWrap(s, t, border)`, `SetCompareMode(func)`/`DisableCompareMode`, `Bind(unit)`, `static Unbind(unit)`. Dzięki temu ta sama tekstura głębi jest w passie światła samplerem porównującym (sprzętowy PCF), a poza nim zwykłą teksturą (podgląd w panelach). **Pamiętaj o `Unbind` przed ImGui** — sampler porównujący zostawiony na slocie renderuje tekstury UI na czarno. |

**Pułapka: feedback loop.** Tekstura zbindowana do samplowania **nie może** być jednocześnie celem renderu. Mapa cieni jest bindowana na slot 0 w passie głównym, a w następnej klatce pass cieni renderuje *do niej* — dlatego `Renderer::UnbindShadowTexture()` zdejmuje **teksturę i sampler** ze slotu i na końcu klatki, i przed pętlą kaskad. Część sterowników (NVIDIA) to toleruje, część (Mesa/iGPU) zgłasza `INVALID_OPERATION` i zostawia mapy niezapisane — a niewyczyszczona mapa głębi czyta się jako „wszystko zasłonięte", czyli **czarna scena**.
| `template<typename T> class ShaderStorageBuffer` | `Renderer/GLShaderStorageBuffer.h` | Wrapper SSBO na dowolny POD `T` (std430). **Header-only, NIE `EngineObject`** — szablonu nie da się zreflektować, więc to zwykły typ (trzymaj jak `Texture`). Move-only, `static_assert(is_trivially_copyable)`. API: `Create(count,usage)`/`CreateFromData`/`CreateFromArray`, `Bind`/`BindBase(binding)`/`BindRange`, `Update`/`SetData` (orphaning), `Resize`, `Map`/`MapRange`/`Unmap`, `GetData`, gettery `GetID`/`GetCount`/`GetSizeBytes`/`IsValid`. |

Uwaga (`ShaderStorageBuffer`): wszystkie metody robią GL → wołać z **render threadu** (main nie ma kontekstu GL). Przy deklarowaniu `T` pamiętaj o std430. **`Vec3` jako `T` jest blokowane `static_assert`em** (12 B w C++ vs 16 B stride tablicy `vec3` w std430) — użyj `Vec4` albo dopadowanego structa; `Vec4`/`Matrix4`/`Vec2` pasują 1:1. Guard aktywny gdy glm jest dostępne (`__has_include(<glm/fwd.hpp>)`).

### Main→Render handoff ImGui

| Symbol | Plik | Opis |
|---|---|---|
| `void RenderingManager::SubmitImGuiDrawData(ImDrawData*)` | `Managers/RenderingManager.h` | API z wątku Main: deep-copy danych rysowania ImGui (po `ImGui::Render()`) do wewnętrznego `TripleBuffer` i publish dla render threadu. Aplikacja sama prowadzi swoją klatkę ImGui (`NewFrame`/budowa UI/`Render`) — silnik nie woła już żadnego `OnImGuiRender()`. |
| `struct ImGuiDrawSnapshot` | `Renderer/ImGuiDrawSnapshot.h` | Snapshot jednej klatki ImGui: `CopyFrom(ImDrawData*)` klonuje `CmdLists` (`ImDrawList::CloneOutput`) i kopiuje listę tekstur do pamięci własnej slotu (bo `GetPlatformIO().Textures` jest przebudowywany co klatkę); `Clear()` zwalnia klony. Wzorzec jak [[`RenderSnapshot`]]. |

Uwaga: backend `ImGui_ImplOpenGL3_*` (Init/NewFrame/RenderDrawData/Shutdown) żyje na **render threadzie** (potrzebuje bieżącego kontekstu GL); backend SDL2 (input/platform) na Main. Flaga `ImGuiBackendFlags_RendererHasTextures` jest ustawiana w `ImGuiRenderState::CreateContext` (wołane z `RenderingManager::InitializeImGuiContext()`) na Main, by atlas był spójny od pierwszej klatki.

| Symbol | Plik | Opis |
|---|---|---|
| `class ImGuiRenderState` | `Renderer/ImGuiRenderState.h` | Cykl życia kontekstu ImGui jako pole `RenderingManager`: `CreateContext(window)` na Main (kontekst + IO/DPI + styl silnika + backend platformowy Win32/SDL3), `InitRendererBackend()`/`ShutdownRendererBackend()` na render threadzie (backend OpenGL3). |
| `class OpenGLRenderState` | `Renderer/OpenGLRenderState.h` | Domyślny stan GL kontekstu renderera (depth test `GL_LESS`, blending, polygon mode, debug output przy kontekście debugowym). Pole `RenderingManager`; `Initialize()` woła render thread zaraz po `MakeGLContextCurrent()` — stan GL jest per-kontekst i obowiązuje na obu platformach. |

---

## DynamicArray (PluSTL) — `PluSTL/Array/Array.h`

Poza podstawami (`PushBack`/`EmplaceBack`/`Reserve`/`Erase`/`Sort`/`Append`/`Find`/`Contains`/`IndexOf`/`Remove`/`RemoveIf`) tablica ma zestaw utilsów. `InvalidIndex` = `static_cast<SizeType>(-1)` — wartość zwracana przez `IndexOf*` i `GetRandomIndex()` przy braku wyniku.

**Losowanie** (silnik z `PluRandom`, patrz niżej):

| Funkcja | Opis |
|---|---|
| `SizeType GetRandomIndex() const` | Losowy indeks; `InvalidIndex` gdy pusto. |
| `T& GetRandomItem()` / `const T&` | Losowy element. **Rzuca `std::out_of_range` na pustej tablicy.** |
| `T* GetRandomItemPtr()` / `const T*` | Jak wyżej, ale `nullptr` zamiast wyjątku. |
| `template T* GetRandomItemIf(Predicate)` | Losowy element spełniający predykat (reservoir sampling — jeden przebieg, zero alokacji); `nullptr` gdy nic nie pasuje. |
| `void Shuffle()` | Tasowanie Fisher-Yates in-place. |

**Szybkie usuwanie** (O(1), **nie zachowuje kolejności** — w odróżnieniu od `RemoveAt`/`Remove`, które przesuwają ogon):

| Funkcja | Opis |
|---|---|
| `void RemoveAtSwap(SizeType index)` | Podmienia z ostatnim i skraca. Rzuca przy złym indeksie. |
| `bool RemoveSwap(const T& value)` | To samo po wartości; `false` gdy nie znaleziono. |

**Zapytania:**

| Funkcja | Opis |
|---|---|
| `bool IsValidIndex(SizeType) const` | Indeks w zakresie. |
| `template SizeType IndexOfIf(Predicate) const` | Indeks pierwszego pasującego; `InvalidIndex` gdy brak. |
| `template bool ContainsIf(Predicate) const` / `Any(Predicate)` | Czy istnieje pasujący element (aliasy). |
| `template bool All(Predicate) const` | Czy wszystkie pasują (pusta tablica → `true`). |
| `template SizeType CountIf(Predicate) const` | Ile pasuje. |
| `SizeType Count(const T&) const` | Ile równych wartości. |
| `Iterator MinElement(Comparator = <)` / `MaxElement(...)` | Iterator do min/max wg komparatora "mniejszości"; `End()` gdy pusto. |
| `template<typename R = T> R Sum() const` | Suma elementów; `R` chroni przed przepełnieniem (np. `Sum<UInt64>()`). |

**Modyfikacja:**

| Funkcja | Opis |
|---|---|
| `bool AddUnique(const T&)` | `PushBack` tylko gdy elementu nie ma; `true` = dodano. |
| `void SwapItems(SizeType a, SizeType b)` | Zamiana dwóch elementów (no-op przy złych indeksach). |
| `void Swap(DynamicArray&)` | Zamiana zawartości dwóch tablic (O(1), zamienia też alokatory). |
| `void Fill(const T&)` | Nadpisuje wszystkie istniejące elementy; **nie zmienia rozmiaru** (najpierw `Resize`). |

**Transformacje** (zwracają nową tablicę, nie modyfikują źródła):

| Funkcja | Opis |
|---|---|
| `template DynamicArray Filter(Predicate) const` | Kopia elementów spełniających predykat. |
| `template auto Map(Func) const` | Mapowanie 1:1; typ wyniku wyprowadzany z funkcji (`DynamicArray<decltype(func(item))>`). |
| `template<typename R> R Reduce(R init, Func) const` | Składanie do jednej wartości od początku: `acc = func(acc, item)`. Typ akumulatora z `init` (np. `Reduce(String(), ...)` scala stringi). |
| `DynamicArray Slice(SizeType start, SizeType count = InvalidIndex) const` | Kopia podzakresu; wyjście poza koniec jest przycinane, nie rzuca. Domyślnie do końca. |
| `DynamicArray First(SizeType count) const` / `Last(SizeType count) const` | Kopia n pierwszych / ostatnich elementów; `count` większe od rozmiaru = cała tablica (**nie** rzuca). Nie mylić z `Front()`/`Back()`, które zwracają referencję do jednego elementu. |

`operator==` / `operator!=` porównują rozmiar i elementy po kolei.

### Random — `PluSTL/Random/Random.h` (`namespace PluRandom`)

Header-only `std::mt19937_64` **thread_local** — losowanie z każdego wątku jest bezpieczne, ale sekwencje nie są współdzielone (a `Seed()` dotyczy tylko bieżącego wątku).

| Funkcja | Opis |
|---|---|
| `void Seed(uint64_t)` | Ziarno silnika bieżącego wątku — deterministyczne losowanie (testy, replay). |
| `uint64_t NextUInt64()` | Surowa liczba z silnika. |
| `size_t NextIndex(size_t size)` | Indeks z `[0, size)`; dla `size == 0` zwraca 0. |
| `int64_t NextInt(min, max)` | Liczba z `[min, max]` **obustronnie domkniętego**; odwrócone limity normalizowane. |
| `float NextFloat(min = 0, max = 1)` | Liczba z `[min, max)`. |
| `bool NextBool(probability = 0.5f)` | Rzut monetą z zadanym prawdopodobieństwem sukcesu. |

Do losowych transformów w edytorze (z jawnym seedem i wsadowym wypełnianiem tablic) jest osobne `Editor/Utils/RandomTransformUtils.h` — patrz sekcja Editor.

---

## String (PluSTL) — `PluSTL/String/String.h`

Statyczne helpery na `BasicString` (`String` / `StringW`). Dostępne jako `String::Nazwa(...)`.

**Konwersje liczbowe → String:**

| Funkcja | Opis |
|---|---|
| `String::FromInt<IntT>(value)` | Liczba całkowita → String. |
| `String::FromFloat<FloatT>(value, int precision = 6)` | Liczba zmiennoprzecinkowa → String. |
| `String::FromBool(bool)` | `"true"` / `"false"`. |
| `String::FromPointer<T>(T* ptr)` | Wskaźnik → String (hex). |

**Parsowanie String → liczba:**

| Funkcja | Opis |
|---|---|
| `str.ToInt<IntT>(bool* success = nullptr)` | String → liczba całkowita. |
| `str.ToDouble(bool* success = nullptr)` | String → `double`. |
| `str.ToFloat(bool* success = nullptr)` | String → `float`. |

**Konwersje szerokość znaku** (pełne UTF-8 ↔ UTF-16/32; narrow = UTF-8, wide = UTF-16 na Windows / UTF-32 na Linux, nieprawidłowe sekwencje → U+FFFD):

| Funkcja | Opis |
|---|---|
| `String::FromNarrow(const char*)` / `FromNarrow(String)` | `char*` (UTF-8) → bieżący typ stringa. |
| `String::FromWide(const wchar_t*)` / `FromWide(StringW)` | `wchar_t*` → bieżący typ stringa. |
| `str.ToWide()` | Bieżący string → `StringW`. |
| `str.ToNarrow()` | Bieżący string → `String` (UTF-8). |
| `Plu::StringEncoding::{DecodeUtf8, EncodeUtf8, DecodeWide, EncodeWide, Utf8EncodedLength, WideEncodedLength}` | Niskopoziomowe helpery kodowania per-codepoint (`String/String.h`). |

**Formatowanie** (placeholdery `{}` lub indeksowane `{0}`):

| Funkcja | Opis |
|---|---|
| `String::Format(fmt, args...)` | Statyczna metoda formatująca. |
| `Plu::Format(const char*/String fmt, args...)` | Wolna funkcja, zwraca `String`. |
| `Plu::FormatW(const wchar_t*/StringW fmt, args...)` | Wolna funkcja, zwraca `StringW`. |

---

## Logowanie — `PluEngine/Log.h`

Makra spdlog. Wersje `PLU_CORE_*` logują na logger silnika, `PLU_*` na logger klienta/gry.
Format z placeholderami `{0}`, `{1}`, …

| Makro | Poziom |
|---|---|
| `PLU_TRACE(...)` / `PLU_CORE_TRACE(...)` | trace |
| `PLU_INFO(...)` / `PLU_CORE_INFO(...)` | info |
| `PLU_WARN(...)` / `PLU_CORE_WARN(...)` | warn |
| `PLU_ERROR(...)` / `PLU_CORE_ERROR(...)` | error |
| `PLU_CRITICAL(...)` / `PLU_CORE_CRITICAL(...)` | critical |

`Plu::Log::Init()` inicjalizuje loggery (wołane raz przy starcie).

## Profilowanie / timery — `PluEngine/Timer.h` + `PluEngine/Profiler.h`

Pomiary czasu trafiają do globalnego rejestru `Profiler` (thread-safe singleton, mapa klucz → historia ostatnich 120 próbek + last/avg/min/max/calls). Podgląd w edytorze: panel **Profiler** (menu View). Każdy pomiar **zawsze** ląduje w rejestrze; log do konsoli jest opcjonalny.

**Wpisy są rozdzielone per wątek.** Klucz mapy to `Profiler::MakeKey(name, threadName)` = `"wątek|nazwa"`, a `ProfilerEntry` niesie `Name` i `ThreadName` osobno — ten sam timer zmierzony na Main i na Render daje dwa niezależne wpisy (wcześniej mieszały się w jeden). Wątek bierze się z `GetCurrentThreadName()` (patrz „Thread affinity"), więc **nowy wątek, który profilujesz, powinien zawołać `RegisterThreadName(...)` na wejściu** — inaczej pokaże się jako `Thread <id>`. Panel ma combo filtrujące po wątku (`All threads` = bez filtra).

**Staraj się stosować te timery często.** Gdy dodajesz lub ruszasz nietrywialną logikę — hot paths, pętle, kroki init/load, cokolwiek co może być wolne — domyślnie owijaj to w timer, zamiast czekać na problem z wydajnością. Są tanie i trafiają do panelu Profiler zamiast zaśmiecać konsolę, więc spokojnie można je zostawiać. Instrumentuj kod, zamiast zgadywać, gdzie idzie czas.

| Makro | Opis |
|---|---|
| `PLU_PROFILE_SCOPE(name)` | Scoped timer (RAII) — mierzy do końca scope'a, zapis tylko do rejestru. |
| `PLU_PROFILE_SCOPE_LOG(name)` | Jak wyżej + log do konsoli (progi TRACE/INFO/WARN wg czasu). |
| `PLU_TIMER_START(name[, logToConsole])` | Ręczny start timera; drugi arg (bool) włącza log do konsoli. |
| `PLU_TIMER_END(name)` | Ręczny stop timera o danej nazwie. |

| Funkcja `Profiler` | Opis |
|---|---|
| `Profiler::GetInstance()` | Singleton rejestru timingów. |
| `Record(name, durationMs)` | Dopisuje pomiar do historii wpisu `(name, bieżący wątek)` (zwykle wołane przez `Timer`). |
| `RecordForThread(name, threadName, durationMs)` | Jak wyżej, ale z jawną nazwą wątku — dla pomiarów zbieranych gdzie indziej niż powstały (np. GPU timery). |
| `Snapshot()` | Kopia rejestru (`GameHashMap<String, ProfilerEntry>`, klucz = `MakeKey`) do bezpiecznego odczytu (np. panel). |
| `SnapshotThreadNames()` | Posortowana `DynamicArray<String>` wątków, z których są pomiary — źródło listy dla filtra w panelu. |
| `Profiler::MakeKey(name, threadName)` | Klucz wpisu: `"wątek\|nazwa"`. |
| `Clear()` | Czyści wszystkie timingi. |
| `BuildCsv(threadFilter = "")` | The registry as CSV text — summary columns plus the sample history unwrapped chronologically into fixed `Sample0..SampleN` columns. Returns text only; pair it with `DiskManager::SaveText`. Empty filter = all threads. |

Export from the UI: the **Profiler** panel's `Export CSV` button asks for a destination and writes `BuildCsv` there, honouring the panel's current thread filter.

Export from a script: `--profiler-export-after <seconds>` makes `Application::Run` write the CSV once, that many seconds after the main loop starts, then carry on running (kill the process or close the window when done). `--profiler-export-path <path>` picks the destination (default `profiler.csv` relative to the working directory). Unlike the panel button there is no thread filter — the dump holds every thread. The arguments are registered by `Application::AddEngineArguments(parser)`, which each executable's `main()` calls before `parse_args`; an app that skips it simply has no such flags (reading them is guarded, not an error).

### GPU timery — `PluEngine/Renderer/GPUProfiler.h`

`PLU_PROFILE_SCOPE*` mierzy tylko czas CPU-side submitu komend GL, nie faktyczne wykonanie na GPU (kolejka komend jest asynchroniczna) — dlatego wszystkie CPU-passy potrafią wyglądać "tanio", a cały realny koszt wypływa dopiero tam, gdzie CPU musi poczekać na GPU (typowo `SwapBuffer`). `GPUProfileScope`/`PLU_PROFILE_SCOPE_GPU` mierzy realny czas GPU przez parę znaczników `GL_TIMESTAMP`. Wynik trafia do tego samego rejestru `Profiler` pod pseudo-wątkiem **`GPU`** (`RecordForThread`), pod własną nazwą sondy — filtr wątku w panelu oddziela je od timerów CPU. Publikacja jest opóźniona o 1-3 klatki (async) — normalne, nie błąd.

| Makro / Funkcja | Opis |
|---|---|
| `PLU_PROFILE_SCOPE_GPU(name)` | Scoped GPU timer (RAII). Tylko wątek renderu (wymaga kontekstu GL). |
| `GPUProfileScope::PollResults()` | Odbiera gotowe wyniki i publikuje je do `Profiler`; wołane raz na klatkę w `RenderingManager::RenderThreadLoop`. |

### "Hottest" assety renderowania — `PluEngine/Renderer/RenderUsageStats.h`

Globalny rejestr `RenderUsageStats` (plain singleton, jak `Profiler`) zliczający które **static meshe** i **tekstury** są najczęściej używane w passie sceny. Zapis i odczyt dzieją się na wątku MAIN (`RenderSnapshotBuilder` liczy — tylko w edytorze, pod `PLU_ENGINE_EDITOR_BUILD`; panel czyta), więc **bez synchronizacji**. Podgląd: panel **Render / GPU** → zakładka **Hottest Assets**. Liczy wyłącznie tekstury materiałów — mapy cieni (kaskady CSM) są silnikowe i **nie** są liczone. Klucz map to surowy `UInt64` UUID; nazwę rozwiązuje panel przez `AssetManager`.

| Funkcja `RenderUsageStats` | Opis |
|---|---|
| `RenderUsageStats::GetInstance()` | Singleton rejestru użycia assetów. |
| `BeginFrame()` | Nowa klatka: `CurrentFrameUses` → `LastFrameUses`, zeruje akumulator. Woła się raz/klatkę. |
| `RecordMesh(uuid)` / `RecordTexture(uuid)` | Zlicza użycie (inkrementuje bieżącą klatkę + sumę). `uuid==0` ignorowane. |
| `GetMeshUsage()` / `GetTextureUsage()` | Const-ref do rejestru (`GameHashMap<UInt64, AssetUsageEntry>`) — odczyt na tym samym wątku co zapis (MAIN). |
| `Clear()` | Zeruje wszystkie liczniki. |

### FPS per-wątek — `PluEngine/PluUtils.h` (`namespace Plu`)

Wątek Main (pętla gry/UI) i wątek Render chodzą niezależnie (rozdzielone przez TripleBuffer `RenderSnapshot`), więc mają różne tempo klatek. Każda pętla publikuje swoją deltę przez setter; gettery zwracają FPS. Wszystko thread-safe (atomiki). Settery są wołane przez silnik (`Application::Run` dla Main, `RenderingManager::RenderThreadLoop` dla Render) — w kodzie zwykle wołasz tylko gettery.

| Funkcja | Opis |
|---|---|
| `float GetMainThreadFPS()` | FPS wątku Main (z ostatniej delty pętli głównej). `PLU_FUNCTION` (Python). |
| `float GetRenderThreadFPS()` | FPS wątku Render (z ostatniej delty render loop). `PLU_FUNCTION` (Python). |
| `void SetMainThreadDeltaTime(float s)` | Publikuje deltę Main (woła silnik — nie ruszaj). |
| `void SetRenderThreadDeltaTime(float s)` | Publikuje deltę Render (woła silnik — nie ruszaj). |

### Liczniki renderu (draw calls / instancje / culling) — `PluEngine/PluUtils.h` (`namespace Plu`)

Ten sam wzorzec co FPS per-wątek: `Renderer::RenderSnapshot` (render thread) liczy realne draw calle podczas rysowania (po batchowaniu/cullingu) i publikuje finalne wartości klatki tu; panel **Render / GPU** (main thread) czyta gettery. Bezpośredni odczyt `RenderSnapshot::StatDrawCalls`/`StatInstancesDrawn`/`StatCulledCount` z main threadu **nie jest bezpieczny** (wyścig z render threadem) — te pola to tylko roboczy akumulator wewnątrz `Renderer::RenderSnapshot`.

| Funkcja | Opis |
|---|---|
| `UInt32 GetStatDrawCalls()` | Realne draw calle ostatniej klatki (main pass **i** pass cieni). `PLU_FUNCTION` (Python). |
| `UInt32 GetStatInstancesDrawn()` | Suma narysowanych instancji (niezależnie od tego, czy poszły jednym `glDrawElementsInstanced`, czy fallbackiem). `PLU_FUNCTION` (Python). |
| `UInt32 GetStatCulledCount()` | Ile instancji odpadło przez culling — kamerowy **plus** per-kaskadowy culling casterów cieni (jeden obiekt liczy się wielokrotnie, gdy wypada z kilku kaskad). `PLU_FUNCTION` (Python). |
| `void SetShadowCascadeStats(const UInt32* counts, UInt32 cascadeCount)` / `UInt32 GetStatShadowCascadeCount()` / `UInt32 GetStatShadowCascadeCasters(UInt32 idx)` | Ten sam mirror, per kaskada cieni: ilu casterów faktycznie przeszło culling do mapy głębi każdej kaskady. Publikuje `Renderer::RenderSnapshot`, czyta panel Render/GPU. |
| `void SetRenderFrameStats(UInt32 drawCalls, UInt32 instancesDrawn, UInt32 culledCount)` | Publikuje liczniki klatki (woła silnik — nie ruszaj). |

## Debug / asercje — `PluEngine/Core.h`

| Makro | Opis |
|---|---|
| `PLU_ASSERT(x, msg...)` | Asercja na loggerze klienta (tylko `PLU_DEBUG`). |
| `PLU_CORE_ASSERT(x, msg...)` | Asercja na loggerze silnika (tylko `PLU_DEBUG`). |
| `PLU_DEBUGBREAK()` | Przerwanie debuggera (`__debugbreak` / `SIGTRAP`); no-op poza debugiem. |

---

## Thread affinity — `PluEngine/Threading/ThreadAffinity.h` (`namespace Plu`)

Identyfikacja wątku głównego dla egzekwowania thread-confinementu (multithreading: core mutowany tylko na main, render czyta snapshot) + nazwy wątków dla diagnostyki.

| Funkcja | Opis |
|---|---|
| `RegisterMainThread()` | Zapisuje bieżący wątek jako główny i nazywa go `"Main"`. Wołane RAZ, na main, w `Application::EngineInit`. |
| `GetMainThreadId()` | `std::thread::id` zarejestrowanego wątku głównego (domyślny id, jeśli nie zarejestrowano). |
| `IsOnMainThread()` | `true`, gdy bieżący wątek == główny. Zwraca `true` także przed rejestracją (brak fałszywych asercji w pre-init/narzędziach). Używaj w `PLU_CORE_ASSERT` do guardów confinementu. |
| `RegisterThreadName(name)` | Nazywa bieżący wątek (thread-local). Wołaj raz, na wejściu wątku — np. `RenderingManager::RenderThreadEnter` ustawia `"Render"`. |
| `GetCurrentThreadName()` | Nazwa bieżącego wątku; nigdy pusta — fallback to `"Main"` dla zarejestrowanego maina, inaczej `"Thread <id>"`. Używane przez `Profiler` do grupowania wpisów. |

Confinement-guarded (prywatny `CheckOwnerThread()` = `PLU_CORE_ASSERT(IsOnMainThread(), ...)`, no-op w release): `EngineAssetManager` — mutacje rejestru assetów tylko na main (etap 03); `EngineObjectManager` — już **nie** main-confined dla create/destroy (slot-mapa chroniona `shared_mutex`, affinity per-obiekt przez wskaźniki — patrz niżej), `CheckOwnerThread` został tylko w wolnym editor-introspekcyjnym `GetAllObjectsOfClass`.

### Kontrakt thread-affinity wskaźników — `PluSTL/Pointers/` (`namespace Plu`)

Każdy obiekt jest **przypięty do wątku, który go stworzył** (`ControlBlock::owningThread`, łapany w konstrukcji bloku = przy pierwszym owinięciu raw ptr, w praktyce wątek wołający `CreateObject`). Kontrakt: **owning operuje się tylko na wątku-właścicielu; między wątkami przekazuje się wyłącznie `TUsePointer` (obserwacja read-only)**. Asercje gated `#if !defined(NDEBUG) && !defined(PLU_DISABLE_PTR_THREAD_CHECKS)` (znikają w release / można wyłączyć definem), przez makro `PLU_PTR_ASSERT_OWNER(control)` (`control==nullptr || this_thread==owningThread`).

| Operacja | Wątek | Uwaga |
|---|---|---|
| `TOwningPointer`: copy ctor/assign (też konwertujące), `operator->`, `operator*`, `Release()` (= dtor, `=nullptr`, `=raw`, oraz move-**assign** który releasuje stary cel), owning `DynamicCast`/`StaticCast` | **tylko wątek-właściciel** | assert przy naruszeniu |
| `TOwningPointer`: `Get()`/`GetRaw()`, `operator bool`, `==`/`!=`, `std::hash`, **move ctor** | dowolny | escape-hatch: surowy odczyt / null-check / tożsamość / transfer |
| `TUsePointer` — cała klasa (trzymanie, kopia, `operator->`, deref) | **dowolny** | kanał cross-thread |

**Uwaga — to lifetime, nie synchronizacja.** `TUsePointer` pozwala bezpiecznie *trzymać i deref'ować* uchwyt z innego wątku, ale **nie chroni pól obiektu przed wyścigiem danych**. Współdzielone dane między wątkami → przez snapshot (`RenderSnapshot`/`TripleBuffer`), nie przez deref use-ptr. Warstwa wskaźników pilnuje tylko refcountu/lifetime i łapie przypadkowe przekroczenie wątku owningiem.

Konsekwencje praktyczne: zasoby GL (`FrameBuffer`/`Texture`) tworzone na render threadzie są render-owned (owningThread=render) — owning działa na renderze, main co najwyżej obserwuje przez `TUsePointer`. `EngineObjectManager` jest re-entrant-safe: `CreateObject` konstruuje obiekt PRZED `unique_lock`, `DestroyObject` odczepia slot pod lockiem i niszczy obiekt PO zwolnieniu locka (`shared_mutex` jest nierekurencyjny).

---

## Physics — `PluEngine/Physics/` (`namespace Plu`)

**BoundingBox** (`Physics/BoundingBox.h`) — `PLU_STRUCT`, pola `Vec2 X/Y/Z` (min/max na każdej osi):

| Funkcja | Opis |
|---|---|
| `String BoundingBox::ToString()` | Tekstowa reprezentacja boxa. |
| `Vec3 BoundingBox::GetCenter() const` | Środek boxa. |
| `Vec3 BoundingBox::GetExtent() const` | Połowa rozmiaru (extent). |
| `Vec3 BoundingBox::FitCamera(Vec3 origin, Vec3 rot, Vec2 aspect, float FOV) const` | Pozycja kamery mieszcząca cały box w kadrze (do "frame selected"). |
| `BoundingBox BoundingBox::Add(const BoundingBox& other) const` | Suma (union) dwóch boxów. |
| `BoundingBox BoundingBox::Multiply(Vec3 multiplier) const` | Skalowanie boxa. |
| `BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh*)` | Box obejmujący static mesh. |
| `BoundingBox CreateBoundingBox(DynamicArray<Vec3> points)` | Box obejmujący zbiór punktów. |

**Raycast** (`Physics/PhysicsWorld.h`, metoda `PhysicsWorld`, `PLU_FUNCTION` — dostępna z Pythona):

| Funkcja | Opis |
|---|---|
| `RaycastHit PhysicsWorld::Raycast(const Vec3& origin, const Vec3& dir, float maxDist = 1000.0f, RaycastDebugSettings debug = {}, const DynamicArray<GameObject*>& ignoredObjects = {})` | Promień w świecie fizyki; zwraca `RaycastHit`. |

`struct RaycastHit { bool Hit; Vec3 HitLocation; float Fraction; GameObject* HitObject; JPH::BodyID PhysicsBodyHit; }`
`struct RaycastDebugSettings { bool DrawDebug; float DrawTime; }` (`DrawTime` 0 = jedna klatka, >0 = sekundy).
`ignoredObjects` — ciała tych obiektów są pomijane (Jolt `IgnoreMultipleBodiesFilter`). Konieczne, gdy promień startuje wewnątrz własnego collidera: Jolt traktuje convex jako solid i trafiłby w siebie na `Fraction == 0` (tak działa `CharacterPuppet::CheckGrounded`).

**Spawn puppeta** (`GameCore/Puppet.h`, `PLU_FUNCTION(PyOverride)`):

| Funkcja | Opis |
|---|---|
| `virtual Vec3 Puppet::GetSpawnOffset() const` | Offset dodawany do lokacji `PlayerStart` przy spawnie. Domyślnie 0; `CharacterPuppet` zwraca `(0, CapsuleHalfHeight + CapsuleRadius, 0)`, więc PlayerStart oznacza podłogę, a nie środek kapsuły. |

**Kolizje static mesh** (`Physics/StaticMeshCollisionBuilder.h`):

| Funkcja | Opis |
|---|---|
| `DynamicArray<MeshCollisionShapeEntry> BuildCollisionShapesForMesh(StaticMesh* mesh, Vec3 scale = Vec3(1.0f))` | Buduje kształty kolizji Jolt z geometrii mesha. **Drogie** — ConvexHull po wszystkich wierzchołkach albo `MeshShape` (budowa BVH) po wszystkich trójkątach. |
| `const DynamicArray<MeshCollisionShapeEntry>* GetOrBuildUnscaledCollisionShapesForMesh(MeshCollisionShapeCache& cache, StaticMesh* mesh)` | Nieskalowane kształty mesha, budowane raz i zapamiętane. Cache jest kluczowany **wyłącznie po meshu** — skale są per-instancja i w praktyce prawie zawsze różne, więc klucz ze skalą nigdy by nie trafiał. Zwraca **wskaźnik do wnętrza cache'a**: unieważnia go kolejna wstawka, więc konsumuj od razu i nie trzymaj między klatkami. |
| `DynamicArray<MeshCollisionShapeEntry> GetOrBuildCollisionShapesForMesh(MeshCollisionShapeCache& cache, StaticMesh* mesh, Vec3 scale = Vec3(1.0f))` | Jak wyżej + skala nałożona przez `JPH::ScaledShape` (zamiast zapiekania w geometrię). Używaj wszędzie, gdzie budujesz wiele ciał pod rząd. Kształty, które nie potrafią wyrazić danej skali (Jolt dopuszcza tylko jednolitą na sferze), spadają na budowę wprost — geometria zawsze poprawna, tracony jest tylko cache. |
| `PhysicsWorld::InvalidateMeshCollisionCache(StaticMesh* mesh = nullptr)` | Zrzuca zbudowane kształty dla mesha (`nullptr` = wszystkie). **Wołaj po każdej zmianie definicji kolizji assetu**, inaczej ciała dalej powstają ze starych kształtów. |

`struct MeshCollisionShapeEntry { JPH::ShapeRefC Shape; Vec3 LocalOffset; }`.

`LocalOffset` jest w **konwencji sub-shape'a Jolta** — podaje się go wprost do `CompoundShapeSettings::AddShape`. Jolt sam dokłada `Shape::GetCenterOfMass()`, więc kształty o origin przesuniętym do COM (`ConvexHull`) mają `LocalOffset == 0`, a Box/Sphere środek bounding boxa. **Przy rysowaniu kształtu bezpośrednio** (debug wireframe/points — trójkąty wychodzą w przestrzeni COM-centered) trzeba dodać `Shape->GetCenterOfMass()` samemu, patrz `PhysicsWorld::…` edit-mode debug draw.

Offset jest w przestrzeni mesha, już przeskalowany argumentem `scale`, ale **nieobrócony** — składając go z transformem komponentu trzeba go obrócić rotacją tego komponentu przed dodaniem do jego pozycji (`PhysicsCompoundShape::Init`).

**Warstwy kolizji** (`Physics/PhysicsLayers.h`, `namespace Plu::CollisionLayers`) — stałe `STATIC = 0`, `DYNAMIC = 1`, `NUM_LAYERS = 2`. Reguły kolizji i filtry broadphase są w `Physics/PhysicsCollisionRules.h` (klasy infrastrukturalne Jolt, nie wołane bezpośrednio).

**Kanały kolizji w stylu UE** (`Physics/CollisionChannels.h`, `namespace Plu`) — data-driven Block/Overlap/Ignore. `enum class CollisionResponse { Ignore, Overlap, Block }`. `struct CollisionProfile { String Name; UInt8 ObjectType; DynamicArray<CollisionResponse> ResponseTo; }` (preset = UE Collision Preset). `struct CollisionConfig { DynamicArray<String> ChannelNames; DynamicArray<CollisionProfile> Profiles; FindProfileIndex(name); NormalizeProfiles(); }`.

| Funkcja | Działanie |
|---|---|
| `CombineResponse(a, b)` | Słabszy z dwóch (`Ignore < Overlap < Block`). |
| `ResolvePairResponse(cfg, profileA, profileB)` | Łączna reakcja pary po indeksach profili (z `CollisionGroup::GetGroupID()`); poza zakresem → `Block`. |
| `BuildDefaultCollisionConfig()` | Wbudowane kanały/presety (WorldStatic, Pawn, Trigger, BlockAll, OverlapAll, NoCollision…). |
| `SaveCollisionConfig(cfg) -> JSON` / `LoadCollisionConfig(JSON) -> CollisionConfig` | (De)serializacja (zapisywana z projektem / `ProjectDefaults.json`). |
| `CollisionConfig& ActiveCollisionConfig()` | Procesowy aktywny config projektu. `PhysicsWorld` czyta go **na żywo**; edytor/runtime ustawiają go przy ładowaniu projektu, panel Project Settings edytuje w miejscu. |

Filtrowanie nie używa `JPH::GroupFilter` (Jolt budowany bez C++ RTTI → nie linkuje) — odbywa się w `OverlapContactListener`: `OnContactValidate` odrzuca pary `Ignore`, `OnContactAdded/Persisted` ustawia `ContactSettings::mIsSensor` dla `Overlap` (event bez blokady). Kanał jest rozstrzygany **per sub-shape** z materiału (patrz niżej), z fallbackiem na `CollisionGroup::GroupID` ciała (mesh, brak materiału).

**Materiał fizyczny per sub-shape** (`Physics/PluPhysicsMaterial.h`, `namespace Plu`) — friction/restitution/kanał są **własnością pod-kształtu, nie ciała** (jedno `JPH::Body` ma jedną wartość, a kształty komponentów są scalane w compound). `struct PhysicsMaterialData { float Friction; float Restitution; UInt32 CollisionProfileIndex; }`. Z tego samego powodu co `GroupFilter` **nie** subklasujemy `JPH::PhysicsMaterial` (brak RTTI → nie linkuje) — zamiast tego pakujemy dane do 64-bitowego `Shape::SetUserData` liścia (odczyt per sub-shape przez `Shape::GetSubShapeUserData`, Jolt forwarduje przez compound/scaled).

| Funkcja | Działanie |
|---|---|
| `PackPhysicsMaterial(data) -> UInt64` | Pakuje materiał do user-data kształtu. Layout: `present:1 (bit63) \| profileIndex:16 \| friction:16 \| restitution:16`, friction/restitution kwantyzowane przy 1e-4 (0..6.5535). |
| `TryUnpackPhysicsMaterial(packed, out) -> bool` | Odpakowuje; `false` gdy brak bitu obecności (kształt bez materiału, np. mesh). |

`PhysicsBodyComponent::MakeMaterialUserData()` buduje user-data z pól `Friction`/`Restitution`/`CollisionProfile` (każdy `GetShape()` woła `SetUserData` na liściu). `OverlapContactListener` łączy materiały pary per-kontakt: friction = `sqrt(fA*fB)`, restitution = `max(rA,rB)` (domyślne reguły Jolt), kanał = `CollisionProfileIndex` materiału lub fallback `GroupID`. **`GameObject::ActiveBody`** (per-obiekt, bo motion type dotyczy całego ciała) decyduje Dynamic/Static.

> Konwersje Jolt ↔ GLM (`ToJPH`, `ToGLM`, …) są w `PluUtils.h` — patrz sekcja wyżej.
> `JoltShapeExtractor` (`Physics/JoltShapeExtractor.h`) ma `protected static` helpery
> `ExtractTriangles` i `JoltToGlm` — dostępne tylko przez dziedziczenie, nie jako wolne API.

## Reflection — `PluEngine/Reflection/` (`namespace Plu`)

> Większość API reflection jest *generowana* (`*.generated.h`, makra `REFLECTION_BODY_*`,
> `GetStaticClass()`) — patrz `REFLECTION.md`. Poniżej tylko ręczne helpery do użycia w kodzie.

**`TypeRegistry`** (`ReflectionBase.h`) — globalny rejestr typów (singleton):

| Funkcja | Opis |
|---|---|
| `TypeRegistry::GetInstance()` | Wskaźnik na globalny rejestr. |
| `registry->GetTypeOfName(const String&)` | `TypeInfo*` po nazwie typu (lub `nullptr`). |
| `registry->GetTypeMap()` | Mapa wszystkich zarejestrowanych typów. |
| `registry->GetEnumByT<T>()` | `EnumInfo*` dla enuma `T`. |
| `registry->GetObjectManager()` / `GetAssetManager()` | Dostęp do managerów z poziomu reflection. |
| `registry->AddType(TypeInfo*)` / `AddEnum<T>(EnumInfo*)` | Rejestracja (zwykle wołane przez kod generowany). |

**`TypeInfo`** (`ReflectionBase.h`) — opis pojedynczego typu:

| Funkcja | Opis |
|---|---|
| `void* Construct() const` | Tworzy instancję typu (asercja gdy `Abstract`). |
| `PropertyInfo* FindProperty(const String&)` | Property po nazwie — hashed lookup across the whole inheritance chain, built lazily and rebuilt whenever any type gains a property. |
| `PropertyInfo* GetTypeUuidProp() const` | Property z UUID typu. |
| `bool IsChildOf(TypeInfo*)` | Czy bezpośredni typ bazowy. |
| `bool IsDerivedOf(TypeInfo*)` | Czy dziedziczy (pełny łańcuch). |
| `bool IsDerivedOfOrSame(TypeInfo*)` | Jak wyżej lub ten sam typ. |
| `nlohmann::json SerializeToJSON(void* obj) const` | Serializuje instancję do JSON. |
| `void* DeSerializeFromJSON(DeserializationContext*, const json&) const` | Tworzy instancję z JSON. |

**Kopiowanie obiektu bez JSON-a** (`ReflectionBase.h`):

| Funkcja | Opis |
|---|---|
| `void CopyReflectedProperties(TypeInfo* type, const void* src, void* dst)` | Kopiuje wszystkie reflektowane właściwości (własne i odziedziczone) pole po polu. Robi to samo co serializacja+deserializacja, ale bez budowy DOM-u JSON, alokacji kluczy i lookupów po nazwie — używane do duplikowania obiektów sceny (PIE). Wskaźniki kopiują się **płytko**: dla uchwytów assetów (`TUsePointer<StaticMesh>`, …) i `TClassPointer` to jest poprawne. Gdyby powstała właściwość wskazująca na **inny obiekt tej samej sceny**, wymagałaby przemapowania na odpowiednik w kopii — dziś takiej nie ma. |
| `PropertyInfo::CopyPtr` | Typowane przypisanie pola, emitowane przez generator. No-op dla typów bez copy-assignment (generator zabezpiecza `if constexpr`). |

**`PropertyInfo`** — `void* GetPtr(void* objectInstance) const` zwraca wskaźnik na pole w instancji.

**`EnumInfo`** — `void AddValue(String name, UInt64 value)`; pola `EnumValues`, `EnumIntSize`.

**`TClassPointer<T>`** (`ClassPointer.h`) — typebezpieczna referencja na klasę (`TypeInfo*` z gwarancją, że dziedziczy po `T`):

| Funkcja | Opis |
|---|---|
| `TypeInfo* GetRawType() const` | Wskazywany typ. |
| `TypeInfo* GetTType() const` | `T::GetStaticClass()`. |
| `operator TypeInfo*()` | Niejawna konwersja do `TypeInfo*`. |

**Inne:**

| Funkcja | Opis |
|---|---|
| `void RegisterPluClass(pybind11::type)` | Rejestruje klasę zdefiniowaną w Pythonie (`PLU_FUNCTION`). |
| `template<typename T> T FromString(const String&)` | Konwersja string → `T`; działa dla każdego `PLU_ENUM` (po nazwie wartości). |

## Serialization — `PluEngine/Reflection/TypeTraits.h` (`namespace Plu`)

Punkt rozszerzeń serializacji/edytora to szablon **`TypeSerializer<T>`** z trzema statycznymi metodami:

```cpp
static nlohmann::json Serialize(void* data);
static void Deserialize(DeserializationContext*, const nlohmann::json&, void* out);
static bool EditorControl(void* value, const String& name);   // widget ImGui, zwraca czy zmieniono
```

Gotowe specjalizacje (działają out-of-the-box dla pól `PLU_PROPERTY`):

- **Liczby:** `int`, `bool`, `Int8/16/64`, `UInt8/16/32/64`, `float`, `double`
- **Stringi/ścieżki:** `String`, `StringW`, `Path`, `PathW`
- **Engine:** `PluUUID`, `TypeInfo*`, enumy (przez wartość-nazwę, w `ReflectionBase.h`)
- **Kontenery / wskaźniki:** `DynamicArray<T>`, `TUsePointer<T>`, `TOwningPointer<T>`, `TClassPointer<T>`
- **Matematyka:** `glm::vec2/3/4`, `glm::quat`

Dla nowego typu, który ma być serializowalny/edytowalny, dopisz specjalizację `TypeSerializer<TwójTyp>`.

**Pomocnicze (free / static, `PLU_API`):**

| Funkcja | Opis |
|---|---|
| `bool TUsePointerAssetUI(void* value, String name, TypeInfo*)` | Widget ImGui do wyboru assetu pod `TUsePointer`. |
| `bool UUIDForAssetUI(void* value, String name, TypeInfo*, PropertyInfo*)` | Widget ImGui dla property typu `PluUUID` wskazującego asset. |
| `TypeSerializer<TypeInfo*>::SerializeFields(TypeInfo*, void*)` | Serializuje wszystkie `PLU_PROPERTY` typu do JSON. |
| `const char* TypeSerializer<TypeInfo*>::FieldName(const JSON& field)` | Reads the `"name"` of one entry of a serialized `"fields"` array without copying it out of the DOM. Returns `nullptr` for a malformed entry, which `FindProperty` turns into a skipped field instead of a throw. Used by both `Deserialize` overloads — a scene load does this once per field per object. |

`struct DeserializationContext { TUsePointer<IShaderManager> shaderManager; TUsePointer<EngineAssetManager> assetManager; TUsePointer<SceneManager> scenesManager; }` — przekazywany do deserializacji, żeby rozwiązywać referencje na assety/sceny.

## Nazwy obiektów w scenie — `PluEngine/GameObject/GameObject.h`, `PluEngine/Scenes/SceneWorld.h`

| Funkcja | Opis |
|---|---|
| `const String& GameObject::GetObjectName()` | Trwała nazwa obiektu w scenie. Pusta tylko dla obiektów utworzonych z pominięciem `SpawnGameObject`. |
| `void GameObject::SetObjectName(const String&)` | Zmiana nazwy (Structure panel; nie wymusza unikalności). |
| `String SceneWorld::MakeDefaultObjectName(TClassPointer<GameObject>)` | `TypeName` + **najniższy wolny** indeks w tej scenie (`Cube0`, `Cube1`, …). Woła się automatycznie w `SpawnGameObject`. |
| `TUsePointer<GameObject> SceneWorld::SpawnGameObjectUnnamed(TClassPointer<GameObject>)` | Spawn **bez** domyślnej nazwy — wołający musi ją nadać sam. `MakeDefaultObjectName` przechodzi po całej scenie, więc spawn N obiektów to O(n²); przy wczytywaniu z JSON-a wynik i tak nadpisuje deserializacja. Używaj tylko tam, gdzie nazwa jest ustawiana zaraz po spawnie. |
| `String SceneWorld::MakeDefaultObjectNameFromBase(const String& base)` | To samo, ale dla dowolnego prefiksu zamiast `TypeName` — dla nazw nadanych ręcznie (duplikat `Tree3` → `Tree4`). |
| `bool SceneWorld::IsObjectNameTaken(const String&)` | Czy nazwa jest już zajęta (łącznie z pending spawns). |

`mObjectName` jest `PLU_PROPERTY`, więc trafia do JSON-a sceny i przeżywa PIE oraz restart edytora.
**Nie mylić z `EngineObject::GetDisplayName()`** — tamto to `TypeName` + procesowe short-term ID, które
rośnie przez całą sesję (stąd „wygórowane" numerki) i nadaje się tylko do logów. Sceny zapisane przed
wprowadzeniem nazw wczytują się bez zmian: brak pola w JSON-ie = zostaje domyślna nazwa ze spawnu.

## Odtwarzanie obiektów sceny — `PluEngine/Scenes/SceneWorld.h`, `PluEngine/Scenes/SceneManager.h`

| Funkcja | Opis |
|---|---|
| `TUsePointer<GameObject> SceneWorld::SpawnGameObjectWithUuid(TClassPointer<GameObject>, PluUUID)` | Spawn z **podanym** UUID (bez domyślnej nazwy). Dla ścieżek odtwarzających obiekt z zachowaniem tożsamości: UUID jest kluczem w `mGameObjects`, w mapach renderable'i i w attachmentach zapisanych jako `parentUuid`. Nie da się tego zrobić po spawnie — UUID musi istnieć przed `OnSetupComponents`. Zajęty UUID = warning i losowy w zamian. |
| `void SceneWorld::FlushPendingDestroys()` | Natychmiast wykonuje odroczoną kolejkę `DeleteGameObject`. Potrzebne, gdy obiekt kasujesz i odtwarzasz **z tym samym UUID** w jednej operacji. Nie wołać z wnętrza ticka (asercja). |
| `void SceneManager::LoadGameObjectFromJSON(TUsePointer<SceneWorld>, JSON)` | Wczytuje jeden obiekt (z komponentami i attachmentem) do żywej sceny. Honoruje `j["uuid"]`, jeśli jest — ścieżki chcące **nowy** obiekt (Duplicate w Structure panelu) nadpisują to pole świeżym UUID-em przed wołaniem. |
| `void SceneManager::ReloadPythonInstances(const DynamicArray<String>& typeNames)` (editor-only) | Hot reload skryptów: odtwarza wszystkie żywe instancje podanych klas Pythona z nowo zaimportowanych klas. `GameObject` leci cały (serializacja → destroy → spawn z tym samym UUID), `GameObjectComponent` wymieniany w miejscu na właścicielu. Zachowuje `PLU_PROPERTY`, nazwę, transform, UUID i attachmenty w obie strony; **nie** zachowuje atrybutów instancji Pythona (`self.x = ...`). Odmawia działania w PIE. |

Wołającym jest `SceneViewport` — kolejkuje nazwy z eventu `"NewPythonType"`
(`TypeRegistry::TypeRegistryEventDispatcher`) i przetwarza je raz na klatkę w `OnUpdate`. Nie rób tego
z samego handlera: event leci ze środka `RegisterPluClass`, czyli w trakcie `RunProjectScripts`, gdy
pozostałe moduły projektu są już wyrzucone z `sys.modules` i jeszcze nie zaimportowane.

## Editor — `Editor/Utils/`

| Funkcja | Plik | Opis |
|---|---|---|
| `bool RGBTransformDrag3(label, p_data, components, v_speed, p_min, p_max, format, flags)` | `RGBTransformDragger.h` | Wieloskładnikowy `DragScalar` z kolorowaniem osi R/G/B (transform widget w ImGui). |
| `float DrawAttachPointMarker(ImDrawList*, const Matrix4& world, float axisLength, bool selected, const std::function<bool(const Vec3&, ImVec2&)>& project, ImVec2 mouse)` | `AttachPointOverlay.h` | Rysuje marker attach pointa szkieletu (romb + kikuty osi RGB pokazujące rotację) w podglądzie 3D. `project` mapuje świat→piksele (`false` = za kamerą). Zwraca kwadrat odległości kursora od markera (`FLT_MAX` poza ekranem) do klikania. |
| `void MarkSkeletonAssetDirty(TUsePointer<EngineAssetManager>, const Skeleton*)` | `AttachPointOverlay.h` | Brudzi asset Skeleton (po jego `Uuid`). **Do każdej edycji attach pointa zamiast `PanelChangedAsset()`** — SkeletalMesh trzyma szkielet tylko przez UUID, więc zabrudzenie assetu viewportu oznaczyłoby mesh i zmiana nigdy nie trafiłaby na dysk. |
| `void TextCentered(const char* text)` | `CenteredText.h` | `ImGui::Text` wyśrodkowany w poziomie względem szerokości bieżącego okna. |
| `void TextCenteredBoth(const char* text)` | `CenteredText.h` | `ImGui::Text` wyśrodkowany w poziomie i pionie względem rozmiaru bieżącego okna. |
| `void GenerateRandomLocations(DynamicArray<Vec3>* out, UInt32 count, const Vec3& min, const Vec3& max, UInt64 seed = RandomTransformSeedAuto)` | `RandomTransformUtils.h` | Losowe lokacje w prostopadłościanie `[min, max]`. |
| `void GenerateRandomRotations(DynamicArray<Vec3>* out, UInt32 count, const Vec3& min, const Vec3& max, UInt64 seed = RandomTransformSeedAuto)` | `RandomTransformUtils.h` | Losowe rotacje w **stopniach** (pitch=X, yaw=Y, roll=Z), per-oś z `[min, max]`. |
| `void GenerateRandomScales(DynamicArray<Vec3>* out, UInt32 count, const Vec3& min, const Vec3& max, UInt64 seed = RandomTransformSeedAuto)` | `RandomTransformUtils.h` | Losowe skale niejednorodne, per-oś z `[min, max]`. |
| `void GenerateRandomUniformScales(DynamicArray<Vec3>* out, UInt32 count, float min, float max, UInt64 seed = RandomTransformSeedAuto)` | `RandomTransformUtils.h` | Losowe skale jednorodne — jeden mnożnik z `[min, max]` na wszystkie osie. |

Wszystkie `GenerateRandom*` **czyszczą** tablicę wyjściową i wypełniają ją `count` wartościami
z przedziału domkniętego. `seed == RandomTransformSeedAuto` (`0`, wartość domyślna) → wynik
nie jest powtarzalny; każdy inny seed daje ten sam wynik przy tych samych argumentach.
`nullptr` w `out` jest ignorowany, odwrócone limity (`min > max`) są normalizowane.

---

## Utrzymanie

Ten plik jest dokumentacją ręczną — **nie jest generowany automatycznie**. Zasady:

1. Dodajesz nowy helper (wolna funkcja util, makro, statyczna metoda pomocnicza,
   konwerter) → dopisz go do odpowiedniej sekcji.
2. Zmieniasz sygnaturę / zachowanie istniejącego helpera → zaktualizuj wpis.
3. Usuwasz helper → usuń wpis.
4. Nowa kategoria helperów (np. osobny plik `*Utils.h`) → dodaj nową sekcję.
