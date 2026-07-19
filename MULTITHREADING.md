# MULTITHREADING.md

Dokumentacja architektury wielowątkowej PluEngine — ściąga do pracy przy wątkach.
Stan: pipeline TripleBuffer+RenderSnapshot (dawny „etap 2") wdrożony i zweryfikowany.

## Architektura w pigułce

Dwa wątki:

| | **Wątek main** | **Wątek renderu** |
|---|---|---|
| Robi | input, logika, scena, fizyka, ImGui (budowa klatki), I/O assetów | wyłącznie GL |
| Właściciel | `EngineObjectManager`-mutacje logiki, rejestr `EngineAssetManager`, `SceneWorld`, Input, Python, ImGuiContext (strona platformowa) | **kontekst GL**, FBO, tekstury, ShaderProgramy, VAO/VBO, backend ImGui OpenGL3 |
| Komunikacja → | `TripleBuffer<RenderSnapshot*>` + `TripleBuffer<ImGuiDrawSnapshot*>` + kolejki Request* | `RequestAssetDataLoad` (prośba o I/O na main) |

Kluczowy fakt: **main NIE ma kontekstu GL** — `Application::Run` woła `AppWindow->ReleaseGLContext()`
tuż po init (Application.cpp ~91). Wywołanie GL z maina to cichy no-op (puste/czarne wyniki, zero błędów).

`Renderer` NIE jest EngineObjectem — to zwykła klasa, żyje jako `static gRenderer` tworzony
i niszczony w całości na wątku renderu (`RenderingManager::RenderThreadEnter/Exit`).

## Przepływ klatki

**Main** (`Application::Run`, kolejność faktyczna):
1. Input Update
2. App OnTick (edytor: budowa całej klatki ImGui + `SubmitImGuiDrawData`)
3. Scenes Update (logika + fizyka)
4. `EngineAssetManager::ProcessPendingLoads()` — drenaż żądań I/O z renderu
5. `RenderSnapshotBuilder::BuildSnapshotAndPublish(dt)` — ekstrakcja stanu sceny do POD + Publish
6. Input EndFrame, frame pacing

**Render** (`RenderingManager::RenderThreadLoop`):
1. `GPUProfileScope::PollResults()` — odbiór async wyników GPU timerów z poprzednich klatek
2. `mSceneTripleBuffer->AcquireReadBuffer(&fresh)` — z flagą świeżości (patrz TripleBuffer niżej)
3. `Tick(fresh)` — drenaż kolejek Request* (tekstury/save'y) ZAWSZE; bookkeeping eviction tylko
   przy świeżym snapshotcie (patrz „Liczniki eviction" niżej)
4. **Tylko gdy snapshot świeży**: `gRenderer->RenderSnapshot(snapshot)`
   (palety skinningu → pass cieni CSM → pass główny → debug geometry).
   Stale snapshot (main nie opublikował nowego) = identyczne dane wejściowe, a FBO sceny trzyma
   poprzedni obraz — scena **nie jest re-renderowana** (oszczędność GPU, gdy render wyprzedza main).
5. `ImGui_ImplOpenGL3_NewFrame` + `AcquireReadBuffer()` ImGui → `RenderDrawData` — co klatkę
   (backbuffer po swapie jest niezdefiniowany, prezentację trzeba powtarzać zawsze)
6. Blit FBO sceny na ekran (gdy ImGui nie renderował) + SwapBuffer

## Klocki

### ThreadAffinity (`Threading/ThreadAffinity.h`)
`RegisterMainThread()` (1. linia EngineInit) / `IsOnMainThread()` / `GetMainThreadId()`.
`IsOnMainThread()` zwraca `true` przed rejestracją — pre-init i narzędzia standalone nie strzelają.
Konwencja detekcji wątku w kodzie silnika: `if (!IsOnMainThread())` = „jestem na renderze".
(Osobnego `IsOnRenderThread()` już NIE ma — usunięty razem z lockstepem.)

Nazwy wątków (diagnostyka, NIE affinity): `RegisterThreadName(name)` / `GetCurrentThreadName()`.
`RegisterMainThread()` nazywa maina `"Main"`, `RenderThreadEnter()` nazywa render thread `"Render"`.
Nazwa jest thread-local i służy `Profilerowi` do rozdzielania wpisów per wątek (panel Profiler ma
filtr po wątku; GPU timery lądują pod pseudo-wątkiem `"GPU"`). Dokładając nowy wątek, który cokolwiek
profiluje, zawołaj `RegisterThreadName` na jego wejściu — inaczej pokaże się jako `Thread <id>`.

### TripleBuffer (`Threading/TripleBuffer.h`)
Lock-free, klasyczny algorytm 3-slotowy. Writer (main): `GetWriteBuffer()` → wypełnij → `Publish()`.
Reader (render): `AcquireReadBuffer(bool* outFresh = nullptr)` — nigdy nie blokuje, przy braku
nowych danych oddaje poprzedni bufor; `outFresh` mówi, czy to nowo opublikowany snapshot (render
thread pomija re-render sceny dla stale'a — patrz „Przepływ klatki").
Telemetria: dropped (main wyprzedza render) / stale-reused (render wyprzedza main),
wystawiona przez `RenderingManager::GetSnapshot*/GetImGui*Count()`, reset `ResetTripleBufferTelemetry()`.
Teardown: gdy `T` jest wskaźnikiem owning (np. `ImGuiDrawSnapshot*`), sloty alokowane leniwie trzeba
zwolnić po zjoinowaniu render threadu przez `GetBuffersForTeardown()` — robi to `~RenderingManager()`.

### RenderSnapshot (`Renderer/RenderThreading.h`)
POD-owy stan klatki: `SkeletalMeshRenderObjects` (UUID mesha/materiału + transform + paleta kości;
static meshe idą WYŁĄCZNIE przez batche instancingu poniżej — płaska lista
`StaticMeshRenderObjects` została usunięta po fazie 3), `DirLight`+`HasDirLight`,
kamera (projekcja, lokacja, rotacja, **`CameraFOV`** — CSM liczy pod-frustumy per-kaskada, sama
projekcja nie wystarcza), debug geometry fizyki (`DebugLineVerts`/`DebugPointVerts` — płaskie
bufory interleaved pos(3)+color(3)).
Budowany przez `RenderSnapshotBuilder::BuildSnapshotAndPublish` (main); render dostaje same
UUID-y i **rozwiązuje zasoby po swojej stronie** (leniwie, patrz niżej).

**Instancing static meshy:** snapshot niesie też `StaticMeshBatches` (klucz mesh+materiał+CastsShadow,
`InstanceOffset`/`VisibleCount`/`TotalCount`), `StaticInstanceData` (płaska tablica `InstanceGPUData`,
indeksowana `gl_InstanceID` na GPU przez SSBO) i `StaticInstanceBounds` (bounds równoległe, do
cullingu). **Grupowanie (bucketing po hashu klucza, `RenderSnapshotBuilder::mBatchLookup`) i —
docelowo — frustum culling to odpowiedzialność wątku MAIN**, w tym samym miejscu co dziś ekstrakcja
komponentów (patrz `BatchStaticMeshes` w `BuildSnapshotAndPublish`) — pass grupujący i tak przechodzi
po wszystkich obiektach, więc culling wpina się tam prawie za darmo. Render thread tylko uploaduje
`StaticInstanceData` do SSBO (`Renderer::mInstanceBuffer`, binding 1, `BindBase` na całą klatkę,
**przed** `RenderShadowPass` — oba passy czytają ten sam upload tej klatki) i rysuje batche.

Główny pass (`Renderer::RenderSnapshot`) jest **opt-in per materiał**: `DrawStaticMeshInstanced`
gdy `ShaderProgram::HasInstanceDataBlock()` (niezależnie od `VisibleCount` — instanced-owy
`BasicVertInstanced.vert` celowo nie ma `uniform mat4 model`, więc dla takich programów fallback
per-obiekt renderowałby zły transform), inaczej fallback per-obiekt bajtowo zgodny ze starą ścieżką
(materiały na starych programach nietknięte). Shadow pass (`RenderShadowPass`) jest **silnikowy,
zawsze instanced** dla static meshy — depth-only geometrię rysuje jeden współdzielony
`OnlyPositionInstancedShader` (nie materiał sceny), więc nie ma tu opt-in: każdy batch z
`CastsShadow` idzie jednym `DrawStaticMeshInstanced` po `TotalCount` instancji zamiast N rysowań.

Liczniki `StatDrawCalls`/`StatInstancesDrawn`/`StatCulledCount` na `RenderSnapshot` to tylko roboczy
akumulator klatki na renderze — main thread (panel edytora) czyta je przez mirror `GetStatDrawCalls()`
itp. (`PluUtils.h`), na wzór `GetRenderThreadFPS()` (patrz HELPERS.md), bo żywy `RenderSnapshot` nie
jest bezpieczny do odczytu cross-thread.

**`InstancedStaticMeshComponent` (faza 3):** rejestruje się we własnej mapie `SceneWorld::mInstancedMeshRenderables`
(dziedziczy z `WorldComponent`, nie z `StaticMeshComponent` — patrz komentarz w jego nagłówku o `PhysicsWorld`).
Na wątku MAIN, w tym samym passie grupującym (`BatchStaticMeshes`/`BatchInstancedStaticMeshes` w
`BuildSnapshotAndPublish`), każda pozycja w `Instances` (macierz świata z `GetInstanceWorldMatrices()`,
cache'owana per-komponent, przebudowywana gdy `Instances` albo world matrix komponentu się zmieni) trafia
przez ten sam dodawacz (`AddInstanceToBatch`) i ten sam klucz `(MeshUUID, MaterialUUID, CastsShadow)` co
auto-batching luźnych `StaticMeshComponent` — więc ISMC i luźne komponenty na tym samym meshu+materiale
scalają się w jeden batch/draw call. Render thread nie wie o istnieniu ISMC w ogóle, widzi tylko
`StaticMeshBatches`/`StaticInstanceData`.

### Affinity wskaźników (PluSTL)
`ControlBlock::owningThread` = wątek, który wołał `CreateObject`. `TOwningPointer` operowalny
(copy/deref/Release) tylko na wątku-właścicielu — assert `PLU_PTR_ASSERT_OWNER` (debug;
wyłączany `PLU_DISABLE_PTR_THREAD_CHECKS`). **Move jest thread-neutralny.** `TUsePointer` =
obserwacja z każdego wątku. `EngineObjectManager`: slot-mapa pod `shared_mutex`, Create/Destroy
z dowolnego wątku, readery use-only.

## Przepisy — „chcę zrobić X przy wątkach"

### GL z maina / panelu edytora (zapis, readback, kompilacja...)
NIE wołaj GL bezpośrednio. Dodaj kolejkę Request* w `RenderingManager` na wzór istniejących:
`RequestTextureFromInfo` / `RequestStaticMeshLoad` / `RequestTextureSave`. Wzorzec:
```
if (!IsOnMainThread()) { /* render — zrób od razu pod lockiem */ }
else { /* main — tylko enqueue (z dedupe), render zdrenuje w Tick() */ }
```
Kolejki tekstur żyją pod `mTextureMutex`; mapy tekstur/meshy są **własnością renderu**.

### Render potrzebuje danych assetu, których nie ma
Na renderze wolno tylko `GetAssetDataNoLoad(uuid)` (bez I/O). Gdy zwróci null:
`RequestAssetDataLoad(uuid)` i spróbuj w następnej klatce — main zrobi I/O w
`ProcessPendingLoads()`. Tak działają shadery silnikowe (OnlyPosition/DebugLine), materiały
i meshe. **Nie hardkoduj pre-warmów** — ten mechanizm jest generyczny.

### Nowy typ obiektu renderowanego
1. Dodaj POD do `RenderSnapshot` (`RenderThreading.h`) + wyczyść w `Clear()`.
2. Ekstrakcja na main w `RenderSnapshotBuilder::BuildSnapshotAndPublish` (tu wolno dotykać
   sceny/Jolta/ObjectManagera).
3. Rysowanie w `Renderer::RenderSnapshot` (tu wolno GL; zasoby leniwie/z Request*).
Wzór dla danych spoza mesh-pipeline'u: debug geometry fizyki (ekstrakcja → płaski bufor → VBO).

### Nowy zasób GL (FBO, VAO, tekstura specjalna)
Twórz i niszcz na wątku renderu. Stały rozmiar → eager w `Renderer::Initialize`
(jak 4 FBO kaskad); zwalnianie w `Renderer::Shutdown` / `RenderThreadExit` **przed**
`ReleaseGLContext`. Manager main-owned trzymający render-owned obiekty musi je zwolnić na
renderze — wzór: `IShaderManager::ReleaseRenderResources()` wołane z `RenderThreadExit`.

### ImGui
Silnik NIE prowadzi klatki ImGui. Aplikacja (main) buduje ją sama i oddaje wynik:
`NewFrame` → UI → `ImGui::Render` → `RenderingManager::SubmitImGuiDrawData(GetDrawData())`
(deep-copy do `ImGuiDrawSnapshot`: CloneOutput draw list + kopia listy tekstur).
Kontekst ImGui tworzy `InitializeImGuiContext()` (main, strona platformowa+DPI);
backend OpenGL3 initowany na renderze w `RenderThreadEnter`.

**Przebudowa atlasu fontów** (zmiana rozmiaru czcionki) wymaga lockstepu:
`BeginImGuiLockstep` / `StepImGuiLockstep` / `EndImGuiLockstep` (patrz komentarze w
`RenderingManager.h` i użycie w `EditorApp::OnTick`). Warunek „pending" MUSI obejmować
`WantDestroyNextFrame`, nie tylko `Status != OK` — trzymaj lockstep aż atlas się ustabilizuje.

## Pułapki (sprawdź zanim się zdziwisz)

- **`cond ? *owningPtr : nullptr` kopiuje TOwningPointer** → tymczasowy owner → assert affinity
  na obcym wątku. Rozbij na `if (!p) return nullptr; return *p;`.
- **`shared_mutex` jest nierekurencyjny** — lock tylko wokół mutacji kontenera; konstrukcja
  obiektu PRZED lockiem, destrukcja PO (`condemned = std::move(...)` pod lockiem, dtor poza).
  Ctor/dtor EngineObjectu może rekurencyjnie wołać manager.
- **GL z maina = cichy no-op**, nie błąd. Puste/czarne wyniki bez asercji → sprawdź wątek.
- **Okno główne żyje dłużej niż wątek renderu.** Render swapuje `AppWindow` co klatkę —
  `WindowsManager::ProcessNewWindows` NIE niszczy okna 0 (tylko `Close()`);
  `RenderingManager::Shutdown` joinuje wątek; shutdown renderingu PRZED `OnShutdown()`.
- **Liczniki eviction w `Tick()` liczą tiki RENDERU** i muszą być zerowane gdy `uses > 0`
  (gałąź `else`), inaczej churn load/unload + recykling GL id (objaw: podgląd tekstury miga
  atlasem czcionek). Bookkeeping biegnie TYLKO przy świeżym snapshotcie (`Tick(fresh)`) — na
  stale'ach scena się nie renderuje, więc liczniki użyć nie dostają bumpów i liczenie
  bezczynności eksmitowałoby wszystko podczas dłuższego stalla maina (breakpoint, modalny dialog).
- **`DynamicArray::Reserve()` ustawia tylko capacity** (`Size()`=0) — pod `glGetTexImage`/memcpy
  użyj `Resize()`.
- Confinement guardy (`PLU_CORE_ASSERT(IsOnMainThread())`) są no-opami w release — brak
  asercji w release ≠ brak problemu.

## Historia i co dalej

Etapy wdrożenia: **0** atomowy ControlBlock + confinement → **1/1b** lockstep (Runtime, potem
edytor; mechanizm `PrepareFrame`/`RenderOneFrame` + handshake condvar — **usunięty**, zastąpiony
przez pipeline) → **2** pipeline TripleBuffer+RenderSnapshot (obecny stan; cienie CSM, debug
fizyki, ImGui handoff — wszystko zweryfikowane runtime 2026-07) → **3** asset streaming worker
(**nie zaczęty**; założenie: worker bez GL/ObjectManagera/Pythona).

Otwarte tematy: VRAM map cieni (4× 4096² DepthOnly ≈ 256 MB) — do przemyślenia obniżenie
rozdzielczości kaskad.
