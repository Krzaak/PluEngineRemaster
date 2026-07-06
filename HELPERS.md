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

Binarne pliki: **`BinaryFileWriter` / `BinaryFileReader`** — scoped (RAII), zamykają plik w destruktorze; `CloseFile()` ręcznie (zwraca `bool` sukcesu). Non-copyable, movable. 256 KB bufor stdio (`setvbuf`). Konstruktor lub `OpenFile()` przyjmuje `Path` **lub** `PathW`. `HasError()` sygnalizuje short write/read. Preferuj zamiast surowego `fopen`/`fwrite`.

| Metoda | Opis |
|---|---|
| `OpenFile(Path\|PathW)` / `CloseFile()` | Otwórz / zamknij (auto-close w dtorze). |
| `Write(const T&)` / `Read(T&)` | Pojedyncza wartość POD (trivially copyable). |
| `WriteArray(const T*, count)` / `ReadArray(T*, count)` | Ciągła tablica POD. |
| `Write(void*, size)` / `Read(void*, size)` | Surowe bajty. |
| `WriteString(String)` / `ReadString(String&)` | String z prefiksem długości (`UInt32` + bajty UTF-8). |
| `IsOpen()` / `HasError()` | Stan pliku / flaga błędu. |

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

Stałe kamery: `kCameraNearClip = 0.1f`, `kCameraFarClip = 100000.0f`, `kShadowFarClip = 500.0f`.

| Funkcja | Opis |
|---|---|
| `DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)` | 8 narożników frustum w przestrzeni świata. |
| `Matrix4 GetLightViewMatrix(const DynamicArray<Vec3>& corners, const Vec3& lightDir)` | Macierz widoku światła dopasowana do narożników. |
| `Matrix4 GetLightProjectionMatrix(corners, lightView, float zOffset = 50.0f)` | Macierz projekcji światła; `zOffset` odsuwa płaszczyznę near. |
| `DynamicArray<float> GetCascadeSplits(int count, float near, float far, float lambda = 0.5f)` | Podział kaskad CSM (`lambda`: 0=liniowy, 1=logarytmiczny). |
| `Matrix4 GetCascadeProjectionMatrix(float fovY, float aspect, float near, float far)` | Projekcja perspektywiczna pod-frustum jednej kaskady. |
| `DynamicArray<ShadowCascadeData> GetCascadedLightMatrices(...)` | Macierze światła (view*proj) dla wszystkich kaskad CSM. |

`struct ShadowCascadeData { Matrix4 viewProj; float splitDistance; }`.

### Wrappery zasobów GL — `PluEngine/Renderer/`

| Symbol | Plik | Opis |
|---|---|---|
| `class Texture` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLTexture.h` | Tekstura 2D: `Create`/`CreateFromInfo`/`CreateDepth`, `Bind(unit)`, streaming mipów, `SaveTexture`. Move-only. |
| `class FrameBuffer` (`PLU_CLASS`, `EngineObject`) | `Renderer/GLFrameBuffer.h` | FBO: `Create`/`CreateWithTexture`/`CreateDepthOnly`, `Bind`/`Resize`/`BlitTo`, `FrameBufferType` (Color/ColorDepth/DepthOnly/DepthStencil). Move-only. |
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

Pomiary czasu trafiają do globalnego rejestru `Profiler` (thread-safe singleton, mapa nazwa → historia ostatnich 120 próbek + last/avg/min/max/calls). Podgląd w edytorze: panel **Profiler** (menu View). Każdy pomiar **zawsze** ląduje w rejestrze; log do konsoli jest opcjonalny.

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
| `Record(name, durationMs)` | Dopisuje pomiar do historii (zwykle wołane przez `Timer`). |
| `Snapshot()` | Kopia rejestru (`GameHashMap<String, ProfilerEntry>`) do bezpiecznego odczytu (np. panel). |
| `Clear()` | Czyści wszystkie timingi. |

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

## Debug / asercje — `PluEngine/Core.h`

| Makro | Opis |
|---|---|
| `PLU_ASSERT(x, msg...)` | Asercja na loggerze klienta (tylko `PLU_DEBUG`). |
| `PLU_CORE_ASSERT(x, msg...)` | Asercja na loggerze silnika (tylko `PLU_DEBUG`). |
| `PLU_DEBUGBREAK()` | Przerwanie debuggera (`__debugbreak` / `SIGTRAP`); no-op poza debugiem. |

---

## Thread affinity — `PluEngine/Threading/ThreadAffinity.h` (`namespace Plu`)

Identyfikacja wątku głównego dla egzekwowania thread-confinementu (multithreading: core mutowany tylko na main, render czyta snapshot).

| Funkcja | Opis |
|---|---|
| `RegisterMainThread()` | Zapisuje bieżący wątek jako główny. Wołane RAZ, na main, w `Application::EngineInit`. |
| `GetMainThreadId()` | `std::thread::id` zarejestrowanego wątku głównego (domyślny id, jeśli nie zarejestrowano). |
| `IsOnMainThread()` | `true`, gdy bieżący wątek == główny. Zwraca `true` także przed rejestracją (brak fałszywych asercji w pre-init/narzędziach). Używaj w `PLU_CORE_ASSERT` do guardów confinementu. |

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

## Editor — `Editor/Utils/`

| Funkcja | Plik | Opis |
|---|---|---|
| `bool RGBTransformDrag3(label, p_data, components, v_speed, p_min, p_max, format, flags)` | `RGBTransformDragger.h` | Wieloskładnikowy `DragScalar` z kolorowaniem osi R/G/B (transform widget w ImGui). |
| `void TextCentered(const char* text)` | `CenteredText.h` | `ImGui::Text` wyśrodkowany w poziomie względem szerokości bieżącego okna. |
| `void TextCenteredBoth(const char* text)` | `CenteredText.h` | `ImGui::Text` wyśrodkowany w poziomie i pionie względem rozmiaru bieżącego okna. |

---

## Utrzymanie

Ten plik jest dokumentacją ręczną — **nie jest generowany automatycznie**. Zasady:

1. Dodajesz nowy helper (wolna funkcja util, makro, statyczna metoda pomocnicza,
   konwerter) → dopisz go do odpowiedniej sekcji.
2. Zmieniasz sygnaturę / zachowanie istniejącego helpera → zaktualizuj wpis.
3. Usuwasz helper → usuń wpis.
4. Nowa kategoria helperów (np. osobny plik `*Utils.h`) → dodaj nową sekcję.
