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
| `Vec3 GetLookAtRotatorDegrees(const Vec3& eye, const Vec3& target)` | Rotator (w stopniach) patrzący z `eye` na `target`. |
| `Vec3 GetRotatedPointWithRadius(const Vec3& center, float radius, float angleDeg, const Vec3& axis)` | Punkt na okręgu o promieniu `radius` wokół `center`, obrócony o `angleDeg` wokół `axis`. |
| `Vec3 GetSphericalOrbitPoint(const Vec3& center, float radius, float yawDeg, float pitchDeg)` | Punkt na sferze orbitalnej (yaw/pitch) — przydatne dla kamer orbitalnych. |
| `void NormalizeVec3Rotation(Vec3* vec)` | Normalizuje każdą oś rotacji do zakresu `[0,360)`. |
| `Vec3 GetLocationFromMatrix(const Matrix4& m)` | Translacja z macierzy transformacji (kolumna `m[3]`). |
| `Vec3 GetScaleFromMatrix(const Matrix4& m)` | Skala z macierzy (długości wektorów bazowych `m[0..2]`). |
| `Vec3 GetRotationFromMatrix(const Matrix4& m)` | Rotacja (Euler w **stopniach**, pitch=X/yaw=Y/roll=Z) z macierzy — baza znormalizowana skalą, `quat_cast` → `eulerAngles`. |
| `Vec4 PackUInt32ToColor(UInt32 id)` | Pakuje 32-bit id (np. obcięty UUID / indeks obiektu) do koloru RGBA `[0,1]` — bajt na kanał (R=bity 0-7 … A=bity 24-31). Do picking framebuffera. `inline`. |
| `UInt32 UnpackColorToUInt32(const Vec4& color)` | Odwrotność `PackUInt32ToColor` — odczytuje id z koloru (z zaokrągleniem, round-trip dokładny dla RGBA8). `inline`. |

`GetForwardVector`, `GetRightVector`, `GetUpVector` oraz funkcje `Clamp*` są oznaczone
`PLU_FUNCTION()` — są reflektowane i dostępne także z Pythona.

## Ścieżki / system — `PluEngine/PluUtils.h` (`namespace Plu`)

| Funkcja | Opis |
|---|---|
| `PathW GetEngineResourcesDir()` | Katalog zasobów silnika (`PLU_PROJECT_ROOT` w dev, obok exe w dystrybucji). |
| `PathW GetExePath()` | Pełna ścieżka do bieżącego pliku wykonywalnego (Win/Linux). `inline`. |
| `Path GetSystemUserPath()` | Katalog domowy użytkownika (`HOME` / `USERPROFILE`). |

## Disk I/O — `PluEngine/Managers/DiskManager.h` (`namespace Plu`)

`DiskManager` (statyczne): `SaveJson(StringW, json)`, `LoadJson(PathW) -> optional<json>`.

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

| Funkcja | Opis |
|---|---|
| `void Skeleton::CreateBonePalette(DynamicArray<TOwningPointer<SkeletonBone>>* out) const` | Płaska paleta kopii kości, **index-aligned z `SkeletalVertex::BoneIndices`**. Kopie samodzielne (`Children` puste) — to bufor skinningu podawany do shadera. Filtruj sloty po `BoneWeights[i] > 0` (index 0 przy pustym slocie ≠ prawdziwa kość 0). |
| `void Skeleton::CreateNodePalette(DynamicArray<TOwningPointer<SkeletonNode>>* out) const` | Paleta kopii **wszystkich** węzłów (kości i zwykłych) w DFS pre-order, z **zachowaną hierarchią** (`Children` na kopiach). Animowalne drzewo robocze do liczenia transformów globalnych; `out[0]` = kopia roota. |
| `Matrix4 SkeletonAttachPoint::GetLocalMatrix() const` | Transform attach pointa względem węzła-rodzica: `translate(RelativeLocation) * rotate(RelativeRotation)` (bez skali). Złóż z globalną macierzą rodzica (`Skeleton::AttachPoints` trzyma je po nazwie), żeby dostać pozycję w świecie. |
| `bool SkeletalMeshComponent::TryGetAttachPointWorldMatrix(const String& name, Matrix4& out)` | Pełna ramka świata attach pointa (`componentWorld * parentNodeGlobal * attachPointLocal`), liczona z **pozy z ostatniego builda snapshotu** — więc śledzi animację i live posing za darmo. `false`, gdy brakuje mesha/attach pointa/rodzica albo snapshot jeszcze nie poszedł. Bierz to zamiast pary `GetAttachPointLocationInWorld`/`GetAttachPointRotationInWorld`, gdy potrzebujesz całej bazy (np. doczepienie obiektu). |

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
| `ParentIndex[i]` / `BoneSlot[i]` | Indeks rodzica (`-1` = root) / slot w palecie skinningu (`-1` = węzeł nie-kość). |
| `LocalMatrix[i]` / `OffsetMatrix[i]` | Bind-pose local jak zaimportowany / inverse bind (identity tam, gdzie `BoneSlot < 0`). |
| `LocalBindTransform[i]` | `LocalMatrix[i]` zdekomponowany raz przy budowie — bind pose w formie, w której pracuje reszta pipeline'u. Fallback dla węzłów, których nie napędza żaden track. |
| `NodeName[i]` / `NameToIndex` | Nazwy do diagnostyki i bindowania — **nie tykać w pętli per-klatka**. |
| `Pose SkeletalMeshComponent::PosedGlobalTransforms` | Poza w przestrzeni szkieletu (root-relative) per **węzeł**, indeksowana indeksem z `SkeletonPoseLayout` (`CachedBonePalette` to wersja tylko-kości, macierzowa, pod shader). Producent: `RenderSnapshotBuilder`. Pusta do pierwszej ewaluacji; przeżywa trafienie w cache pozy. |

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
| `GraphNode` (`PLU_STRUCT(Abstract)`) | Baza node'a: `PluUUID Uuid`, `InputPins`/`OutputPins`, `virtual void BuildPins()`, `virtual String GetDisplayName()`, `void BuildDataPinsFromReflection()` (dodaje Data-piny z `PLU_PROPERTY` typów: float/double/bool/int/Int*/UInt*/Vec2-4), `NodePin* FindPin(name, dir)`. |
| `NodeGraph : IAssetData` (`PLU_STRUCT`) | Właściciel: `DynamicArray<TOwningPointer<GraphNode>> Nodes` + `DynamicArray<NodeLink> Links`. API: `AddNode(TypeInfo*)`, `RemoveNode(uuid)`, `Connect(fromNode,fromPin,toNode,toPin)` (waliduje + 1 źródło na input), `Disconnect(link)`, `FindNode(uuid)`, `GetLinkSource(toNode,toPin)` (węzeł zasilający dany pin wejściowy, `nullptr` gdy odłączony — baza pod traversal), `RebuildAllPins()`, `virtual TypeInfo* GetNodeBaseType()` (paleta). |
| `NodeGraphSerializer::Save(NodeGraph&, JSON&)` / `Load(dc, NodeGraph&, JSON&)` | Polimorficzny zapis/odczyt nodeów (`typeName`+`fields` przez `TypeSerializer<TypeInfo*>`) + linków. Wołać z loadera assetu (patrz `AnimationGraphAssetLoader`). |

Edytorowa warstwa canvasu (reużywalna, editor-only): `Editor/NodeGraph/` — `NodeGraphEditor::Draw(graph, onModified)` (rysowanie/łączenie/usuwanie/paleta/selekcja/layout), `NodeViewRegistry` + `INodeView`/`DefaultNodeView` (custom rysowanie per typ node'a). Pozycje nodeów = sidecar `<asset>.layout.json`, poza runtime assetem.

### AnimGraph — runtime ewaluacji (`PluEngine/AssetTypes/AnimationGraph/`)

Traversal + sampling/blend, zaimplementowane 2026-07-21 (wcześniej stuby). Bezstanowe: każde wywołanie liczy pozę od zera z `AnimEvalContext::TimeSeconds`, nic nie jest cache'owane na węźle/grafie (state machines / per-instance state = przyszłość).

| Funkcja / typ | Opis |
|---|---|
| `struct AnimEvalContext { float TimeSeconds; bool Loop; TUsePointer<Skeleton> TargetSkeleton; NodeGraph* Graph; }` | Nie-reflected, budowany na nowo per wywołanie ewaluacji przez wołającego (np. przyszły tick `SkeletalMeshComponent`). `Graph` ustawia `AnimationGraph::Evaluate` — nie wypełniać ręcznie. `TargetSkeleton` może być pusty (podgląd grafu bez szkieletu) — nody wtedy zwracają pustą pozę. |
| `Pose AnimGraphNode::EvaluateInputPose(AnimEvalContext&, const String& pinName) const` (protected) | Idzie po linku wpiętym w `pinName` do węzła źródłowego i woła jego `Evaluate`. Pin odłączony / źródło nie jest `AnimGraphNode`: fallback = bind pose z `TargetSkeleton->GetPoseLayout()` (albo pusta poza, gdy brak szkieletu). Tego używa każdy konkretny node zamiast ręcznego `GetLinkSource`+`dynamic_cast`. |
| `Pose AnimationGraph::Evaluate(AnimEvalContext&)` | Punkt wejścia: liniowo szuka `AnimOutputPoseNode` w `Nodes`, ustawia `context.Graph = this`, zwraca jego `Evaluate` (rekursywnie ciągnie graf w górę). Pusta poza gdy brak output node'a. |
| `AnimSampleNode::Evaluate` | Sekundy z kontekstu → ticki (`* Animation::FramesPerSecond`), `fmod`/clamp wg `context.Loop`, potem `Animation::GetTrackBinding(*skeleton)` indeksowany po `SkeletonPoseLayout` — dokładnie wzorzec z `RenderSnapshotBuilder`. Start od `layout.MakeBindPose()`, nadpisywane per-node tylko gdzie jest track. |
| `AnimBlendNode::Evaluate` | `EvaluateInputPose` na pinach `"A"`/`"B"`, `BlendPoses(a, b, Alpha, result)`. |
| `AnimOutputPoseNode::Evaluate` | `return EvaluateInputPose(context, "Pose")`. |

**Podpięte do renderowania (2026-07-21):** `SkeletalMeshComponent` ma `PLU_PROPERTY() TUsePointer<AnimationGraph> AnimGraph` obok istniejącego `AnimationToShow` — **graf ma priorytet, gdy przypisany**, ale surowa animacja NIE jest kasowana ani ignorowana na stałe: odpięcie grafu (`AnimGraph = nullptr`) wraca od razu na `AnimationToShow`. Osobny licznik czasu `float GraphTimeSeconds` (runtime-only, jak `AnimationTimeTicks`) — graf nie ma jednego wspólnego FPS jak pojedyncza animacja, więc `AnimEvalContext::TimeSeconds` jedzie osobno; `OnUpdate` posuwa oba liczniki niezależnie, gdy `IsPlaying`. `RenderSnapshotBuilder.cpp` (~linia 436, `"Skeletal Mesh Calculations"`): gałąź `if (animGraph) { ...AnimationGraph::Evaluate → lokalna poza → BoneLocalOverrides → SkeletonPoseLayout::ComposeGlobals... } else { /* stara pętla sample-and-compose dla AnimationToShow */ }`, obie kończą się w `layout.BuildBonePalette`. Cache pozy (`CachedPoseAnimUuid`/`CachedPoseTicks`) klucz teraz źródło-agnostyczny (`poseSourceUuid`/`poseTimeKey` = uuid+czas grafu **albo** animacji, którykolwiek aktywny).

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

Stałe kamery: `kCameraNearClip = 0.1f`, `kCameraFarClip = 100000.0f`, `kShadowFarClip = 300.0f`.

| Funkcja | Opis |
|---|---|
| `DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)` | 8 narożników frustum w przestrzeni świata. |
| `Matrix4 GetLightViewMatrix(const DynamicArray<Vec3>& corners, const Vec3& lightDir)` | Macierz widoku światła dopasowana do narożników. |
| `Matrix4 GetLightProjectionMatrix(corners, lightView, float zOffset = 50.0f)` | Macierz projekcji światła; `zOffset` odsuwa płaszczyznę near. |
| `DynamicArray<float> GetCascadeSplits(int count, float near, float far, float lambda = 0.5f)` | Podział kaskad CSM (`lambda`: 0=liniowy, 1=logarytmiczny). |
| `Matrix4 GetCascadeProjectionMatrix(float fovY, float aspect, float near, float far)` | Projekcja perspektywiczna pod-frustum jednej kaskady. |
| `DynamicArray<ShadowCascadeData> GetCascadedLightMatrices(...)` | Macierze światła (view*proj) dla wszystkich kaskad CSM. Ostatni parametr to `DynamicArray<float>` rozdzielczości map cieni **per kaskada** (texel snapping; kaskady mogą mieć różne rozdzielczości). |

`struct ShadowCascadeData { Matrix4 viewProj; float splitDistance; }`.

### Frustum culling — `PluEngine/Renderer/RenderUtils.h`

| Funkcja | Opis |
|---|---|
| `Frustum ExtractFrustumPlanes(const Matrix4& viewProj)` | 6 płaszczyzn frustum (Gribb-Hartmann, znormalizowane) z macierzy view*proj. `struct Frustum { Vec4 Planes[6]; }` — `(nx,ny,nz,d)`, wewnątrz gdy `dot(n,p)+d >= 0`. |
| `bool SphereInFrustum(const Frustum&, const Vec3& center, float radius)` | Test sfera-vs-frustum (6 testów płaszczyzna-punkt). |

### Static mesh: draw calls i bounding box — `PluEngine/AssetTypes/StaticMesh/StaticMesh.h`, `PluEngine/Physics/BoundingBox.h`

| Symbol | Opis |
|---|---|
| `void DrawStaticMesh(const StaticMesh*, RenderingManager*)` | Jeden `glDrawElements`. |
| `void DrawStaticMeshInstanced(const StaticMesh*, RenderingManager*, UInt32 instanceCount)` | Jeden `glDrawElementsInstanced` — `instanceCount` instancji naraz, dane per-instancja idą przez SSBO `InstanceMatrices` (indeks `gl_InstanceID` + uniform `instanceBaseIndex`, patrz `Renderer::RenderSnapshot` i `Renderer::RenderShadowPass`). Wywołuje `OnStaticMeshRender` **raz**, nie N razy (to flaga żywotności dla eviction, nie licznik populacji). Programy z blokiem `InstanceMatrices` (`BasicVertInstanced.vert`, `OnlyPositionInstanced.vert`) celowo **nie mają** `uniform mat4 model` — dla nich to jedyna poprawna ścieżka rysowania, niezależnie od liczby instancji. |
| `EngineAssets::OnlyPositionInstancedShader` | Shader głębi (SSBO `InstanceMatrices`) dla static meshy w mapach cieni (`Renderer::RenderShadowPass`) — silnikowy, **zawsze** instanced (nie opt-in per materiał jak główny pass, bo depth pass nie używa materiału sceny). Zastąpił dawny `OnlyPositionShader` (ten drugi zostaje jako nieużywany plik/asset, celowo nieusunięty). |
| `BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh*)` | Chodzi po **każdym wierzchołku** — nigdy per klatka, cache'uj (patrz `StaticMeshComponent::MeshBoundingBoxComputed` / `InstancedStaticMeshComponent`). |
| `StaticMeshComponent::MeshBoundingBox` / `MeshBoundingBoxComputed` | Bounding box (local space) komponentu; `MeshBoundingBoxComputed` to twardy guard — liczony raz w `SetStaticMesh` (jeśli mesh już załadowany) albo leniwie w `RenderSnapshotBuilder`, gdy mesh dojedzie asynchronicznie. |

### Introspekcja shaderów — `PluEngine/Shaders/ShaderProgram.h`

| Symbol | Opis |
|---|---|
| `bool ShaderProgram::HasBoneMatricesBlock()` | Czy zlinkowany program deklaruje blok SSBO `BoneMatrices` (vertex skinning). GL query cache'owane per link (reset przy `UnloadProgram`/`LoadFromBinary`); wołać z **render threadu** po `IsLoaded()`. Renderer używa tego do warninga, gdy skeletal mesh dostaje materiał bez skinningu (mesh stałby w bind pose po cichu). |
| `bool ShaderProgram::HasInstanceDataBlock()` | Jak wyżej, dla bloku SSBO `InstanceMatrices` (instancing static meshy). Renderer używa tego do wyboru `DrawStaticMeshInstanced` vs fallback per-obiekt (opt-in: materiał na programie bez tego bloku renderuje się identycznie jak dziś). |
| `ShaderProgram::Set*Uniform(...)` | Settery uniformów: lokacja cache'owana per nazwa (jeden lookup `Find`), **no-op bez żadnego wywołania GL**, gdy uniform nie istnieje w programie (lokacja -1) — ustawianie uniformów globalnych na wszystkich programach jest tanie. `SetTextureUniform` przy braku uniformu **nie binduje** tekstury. |
| `static ShaderProgram::ResetBindCache()` | `ShaderProgram::Bind()` deduplikuje `glUseProgram` cache'em aktualnie zbindowanego programu (render thread only). Wołać po każdym miejscu, które binduje/kasuje program **poza** `ShaderProgram::Bind` (np. backend ImGui) — `Renderer::RenderSnapshot` robi to na starcie każdej klatki, `UnloadProgram` przy kasowaniu. |

**Punkty bindingu SSBO (silnikowa konwencja, nie zmieniać bez powodu):** `0` = `BoneMatrices` (skinning, `BasicVertSkeletal.vert`/`OnlyPositionSkeletal.vert`), `1` = `InstanceMatrices` (instancing, `BasicVertInstanced.vert`). Oba bindowane `BindBase` przez `Renderer` (instancje raz na klatkę; palety kości per obiekt przez `UploadSkeletalPalette`, **po** ewentualnym `Resize` — `Resize` tworzy nowe ID bufora, a indeksowany punkt trzymałby skasowany bufor). Palety skinningu wszystkich skeletal meshy liczy raz na klatkę `Renderer::BuildSkeletalPalettes` (płaski scratch + zakresy per obiekt).

### Wrappery zasobów GL — `PluEngine/Renderer/`

| Symbol | Plik | Opis |
|---|---|---|
| `class Texture` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLTexture.h` | Tekstura 2D: `Create`/`CreateFromInfo`/`CreateDepth` (opcjonalne `Use16Bit` = D16 zamiast D32F, np. mapy cieni), `Bind(unit)`, streaming mipów, `SaveTexture`. Move-only. |
| `class FrameBuffer` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLFrameBuffer.h` | FBO: `Create` (opcjonalne `Use16BitDepth` dla DepthOnly)/`CreateWithTexture`/`CreateDepthOnly`, `Bind`/`Resize`/`BlitTo`, `FrameBufferType` (Color/ColorDepth/DepthOnly/DepthStencil). Move-only. |
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
| `UInt32 GetStatDrawCalls()` | Realne draw calle ostatniej klatki (main pass). `PLU_FUNCTION` (Python). |
| `UInt32 GetStatInstancesDrawn()` | Suma narysowanych instancji (niezależnie od tego, czy poszły jednym `glDrawElementsInstanced`, czy fallbackiem). `PLU_FUNCTION` (Python). |
| `UInt32 GetStatCulledCount()` | Ile instancji odpadło przez frustum culling. `PLU_FUNCTION` (Python). |
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
| `RaycastHit PhysicsWorld::Raycast(const Vec3& origin, const Vec3& dir, float maxDist = 1000.0f, RaycastDebugSettings debug = {})` | Promień w świecie fizyki; zwraca `RaycastHit`. |

`struct RaycastHit { bool Hit; Vec3 HitLocation; float Fraction; GameObject* HitObject; JPH::BodyID PhysicsBodyHit; }`
`struct RaycastDebugSettings { bool DrawDebug; float DrawTime; }` (`DrawTime` 0 = jedna klatka, >0 = sekundy).

**Kolizje static mesh** (`Physics/StaticMeshCollisionBuilder.h`):

| Funkcja | Opis |
|---|---|
| `DynamicArray<MeshCollisionShapeEntry> BuildCollisionShapesForMesh(StaticMesh* mesh, Vec3 scale = Vec3(1.0f))` | Buduje kształty kolizji Jolt z geometrii mesha. |

`struct MeshCollisionShapeEntry { JPH::ShapeRefC Shape; Vec3 LocalOffset; }`.

`LocalOffset` jest w **konwencji sub-shape'a Jolta** — podaje się go wprost do `CompoundShapeSettings::AddShape`. Jolt sam dokłada `Shape::GetCenterOfMass()`, więc kształty o origin przesuniętym do COM (`ConvexHull`) mają `LocalOffset == 0`, a Box/Sphere środek bounding boxa. **Przy rysowaniu kształtu bezpośrednio** (debug wireframe/points — trójkąty wychodzą w przestrzeni COM-centered) trzeba dodać `Shape->GetCenterOfMass()` samemu, patrz `PhysicsWorld::…` edit-mode debug draw.

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
| `PropertyInfo* FindProperty(const String&)` | Property po nazwie. |
| `PropertyInfo* GetTypeUuidProp() const` | Property z UUID typu. |
| `bool IsChildOf(TypeInfo*)` | Czy bezpośredni typ bazowy. |
| `bool IsDerivedOf(TypeInfo*)` | Czy dziedziczy (pełny łańcuch). |
| `bool IsDerivedOfOrSame(TypeInfo*)` | Jak wyżej lub ten sam typ. |
| `nlohmann::json SerializeToJSON(void* obj) const` | Serializuje instancję do JSON. |
| `void* DeSerializeFromJSON(DeserializationContext*, const json&) const` | Tworzy instancję z JSON. |

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

`struct DeserializationContext { TUsePointer<IShaderManager> shaderManager; TUsePointer<EngineAssetManager> assetManager; TUsePointer<SceneManager> scenesManager; }` — przekazywany do deserializacji, żeby rozwiązywać referencje na assety/sceny.

## Nazwy obiektów w scenie — `PluEngine/GameObject/GameObject.h`, `PluEngine/Scenes/SceneWorld.h`

| Funkcja | Opis |
|---|---|
| `const String& GameObject::GetObjectName()` | Trwała nazwa obiektu w scenie. Pusta tylko dla obiektów utworzonych z pominięciem `SpawnGameObject`. |
| `void GameObject::SetObjectName(const String&)` | Zmiana nazwy (Structure panel; nie wymusza unikalności). |
| `String SceneWorld::MakeDefaultObjectName(TClassPointer<GameObject>)` | `TypeName` + **najniższy wolny** indeks w tej scenie (`Cube0`, `Cube1`, …). Woła się automatycznie w `SpawnGameObject`. |
| `String SceneWorld::MakeDefaultObjectNameFromBase(const String& base)` | To samo, ale dla dowolnego prefiksu zamiast `TypeName` — dla nazw nadanych ręcznie (duplikat `Tree3` → `Tree4`). |
| `bool SceneWorld::IsObjectNameTaken(const String&)` | Czy nazwa jest już zajęta (łącznie z pending spawns). |

`mObjectName` jest `PLU_PROPERTY`, więc trafia do JSON-a sceny i przeżywa PIE oraz restart edytora.
**Nie mylić z `EngineObject::GetDisplayName()`** — tamto to `TypeName` + procesowe short-term ID, które
rośnie przez całą sesję (stąd „wygórowane" numerki) i nadaje się tylko do logów. Sceny zapisane przed
wprowadzeniem nazw wczytują się bez zmian: brak pola w JSON-ie = zostaje domyślna nazwa ze spawnu.

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
