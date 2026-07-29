# Editor — CLAUDE.md

Wskazówki dla pracy w `Editor/`. Uzupełnia główny `CLAUDE.md` z roota repo.

## Zabrudzanie assetów (dirty marking) w viewportach i panelach

**Reguła:** każda mutacja danych assetu zrobiona przez viewport albo panel **musi** oznaczyć asset jako brudny. Dzięki temu edytor pokazuje stan „niezapisane zmiany" (ikona `UnsavedDocument` na oknie, popup „Assets are unsaved" przy zamknięciu), a Ctrl+S / „Save All" wie co zapisać.

Dwie metody do tego:
- `IEditorViewport::ViewportChangedAsset()` — woła z poziomu viewportu; brudzi asset viewportu (`mAsset`).
- `IEditorPanel::PanelChangedAsset()` — woła z poziomu panelu; deleguje do `ViewportChangedAsset()` rodzica.

Obie sprowadzają się do `EngineAssetManager::MarkAssetDirty(...)`.

### Jak to stosować

- Większość kontrolek refleksji zwraca `bool` informujący o zmianie — owijaj w `if`:
  ```cpp
  if (TypeSerializer<Vec3>::EditorControl(&data, "Kolor")) {
      PanelChangedAsset();
  }
  ```
  Tak samo surowe ImGui: `if (ImGui::ColorEdit3(...)) PanelChangedAsset();`, `if (ImGui::Checkbox(...)) ...`.
- Akcje bez wartości zwrotnej (przyciski, spawn/usuń obiekt, dodaj/usuń sub-shape) — wołaj zabrudzacz zaraz po wykonaniu mutacji.
- Mutacje przez zdarzenia (np. spawn/destroy GameObjectów w scenie) lepiej łapać raz, subskrybując event świata, niż rozsiewać wołania po wielu miejscach. Patrz `SceneViewport::OnOpened` — subskrypcja `"GameObjectsChanged"` → `ViewportChangedAsset()`.

### Czego NIE brudzić

- Stanu czysto podglądowego, który nie jest częścią danych assetu (np. `StaticMeshViewport::Material` używany tylko do podglądu, checkbox „Show Collision", kamera viewportu). To ustawienia widoku, nie asset.

## Nazwy obiektów w Structure panelu

Lista bierze `GameObject::GetObjectName()` (trwała nazwa z JSON-a sceny), **nie**
`EngineObject::GetDisplayName()`. Zmiana nazwy: podwójny klik w wiersz albo „Rename" z menu
kontekstowego → `InputText` w miejscu wiersza; Enter/klik poza polem zatwierdza, Esc anuluje.
`CommitRename` ignoruje nazwę pustą i niezmienioną, więc Esc nie brudzi assetu.

Pułapka przy duplikowaniu: serializacja obiektu wiezie w JSON-ie rzeczy, których klon **nie** ma
odziedziczyć. Wszystkie ścieżki „Duplicate" wołają dlatego `SceneStructurePrepareClone(j, world, source)`
przed `LoadGameObjectFromJSON`, a nie składają kroków po swojemu. Funkcja robi trzy rzeczy:

- **nazwa** (`PLU_PROPERTY`, więc jedzie w JSON-ie) → kolejny wolny numerek *tego samego prefiksu*
  (`SceneWorld::MakeDefaultObjectNameFromBase`), więc duplikat `Tree3` nazywa się `Tree4`, a nie
  domyślnym `StaticMeshActor7`;
- **UUID** → świeży. `LoadGameObjectFromJSON` honoruje `uuid` z JSON-a (to on trzyma tożsamość obiektu
  przy wczytywaniu sceny i hot reloadzie Pythona), więc bez podmiany klon wszedłby pod UUID oryginału;
- **attachment** → wyrzucony, klon wychodzi wolny. Samo wyrzucenie nie wystarcza: `location`/`rotation`/
  `scale` w JSON-ie są **względne** wobec rodzica, więc klon stanąłby w tym offsecie licząc od środka
  świata. Transform przepisujemy więc na światowy z oryginału.

Przy duplikowaniu N razy wołaj to w każdej iteracji — poprzedni klon zajął już swój numerek i UUID.
Dodając nową ścieżkę duplikowania przez serializację, użyj tej funkcji.

Kolejność listy: grupy po prefiksie nazwy (alfabetycznie), a w grupie **malejąco** po numerku —
`Tree5` na górze, `Tree0` i nazwy bez numerka na dole (`SceneStructureSortByName`). Sortowane są
razem tablice obiektów i nazw, bo indeks wiersza jest kotwicą zaznaczenia (Shift+klik).

Na wierzchu sortowania po nazwie siedzi **hierarchia attachmentów** (`SceneStructureOrderByAttachment`):
obiekt podpięty przez `GameObject::AttachToObject`/`AttachToComponent` ląduje zaraz pod swoim
rodzicem, o jeden poziom wcięcia głębiej (`mListDepths`), a nazwy porządkują już tylko rodzeństwo.
Funkcja **wyłącznie przestawia** tablice — każdy obiekt świata wychodzi z niej dokładnie raz (sierota
albo obiekt w cyklu ląduje na poziomie roota), bo indeks wiersza dalej jest kotwicą zaznaczenia.
Przeciągnięcie wiersza na inny podpina (`KeepWorld`), upuszczenie na pustą przestrzeń pod listą albo
„Detach" z menu kontekstowego odpina. Każda taka zmiana ustawia `mListDirty` — attachment zrobiony
z kodu/Pythona tego nie robi, więc lista pokaże go dopiero po najbliższej odbudowie.

**Lista jest cache'owana między klatkami** (`mListObjects`/`mListNames`) — budowanie jej co klatkę
to przy tysiącu obiektów ~1 ms (kopia tablicy + kopia nazw + sortowanie). Odbudowa (`mListDirty`)
leci z trzech źródeł:
- subskrypcja `"GameObjectsChanged"` na **aktualnym** świecie (`EnsureSubscribedTo` przepina ją,
  gdy `GetCurrentWorld()` się zmieni — PIE/overlay podmieniają świat pod panelem),
- `CommitRename` (zmiana nazwy z poziomu panelu),
- `SceneObjectDetailsPanel` — `mObjectName` jest `PLU_PROPERTY`, więc da się je zmienić
  w inspektorze z pominięciem `SetObjectName`; panel porównuje nazwę przed/po `EditorControl`
  i przy zmianie dispatchuje `"GameObjectsChanged"`.

**Dokładając nową ścieżkę, która zmienia zestaw obiektów albo ich nazwy poza `SpawnGameObject`/
`DeleteGameObject`, zadispatchuj `"GameObjectsChanged"` na świecie** — inaczej Structure pokaże
nieaktualną listę. Wiersze z martwym `TUsePointer` są pomijane i brudzą cache (siatka
bezpieczeństwa, nie zamiennik eventu).

## Zaznaczanie GameObjectów (primary + multi-selection)

`gEditorAppContext->EditorState.SelectedGameObject` pozostaje **pojedynczym, globalnym**
zaznaczeniem — to je czyta details panel, gizmo, Delete w `SceneViewport` i „Fit In View".
Multi-selection **nie** jest globalny: żyje lokalnie w `SceneStructurePanel::mSelectedObjects`.

Reguła synchronizacji (`SceneStructurePanel::SyncSelection`, wołana co klatkę):
- `SelectedGameObject` (primary) **zawsze** jest jednym z elementów `mSelectedObjects`.
- Jeśli primary został ustawiony **poza panelem** (klik w viewporcie) i nie ma go w tablicy →
  multi-selection kasuje się do tego jednego obiektu. Zaznaczenie z zewnątrz wygrywa, bo
  inaczej podświetlenie w Structure rozjechałoby się z tym, co edytuje details panel.
- Handle nieżywych obiektów są wyrzucane co klatkę (usunięcie Deletem, przeładowanie sceny).

Sterowanie: klik = pojedynczo, Ctrl+klik = toggle, Shift+klik = zakres od kotwicy
(`mSelectionAnchor`, indeks w liście wierszy panelu).

### Kasowanie

Delete łapie `SceneViewport::OnUpdate` (całe okno, niezależnie od tego, który panel ma focus),
ale **samo kasowanie oddaje `SceneStructurePanel::DeleteSelectedObjects()`** — multi-selection
żyje tylko w panelu, więc viewport nie ma skąd wziąć pełnej listy. Ta sama metoda siedzi pod
„Delete" w menu kontekstowym panelu. Gdy panelu nie ma (nie znalazł go `GetPanelSlow`), viewport
spada na starą ścieżkę „skasuj primary".

`DeleteGameObject` jest **odroczone** (`mObjectsToDestroy` przetwarza się na końcu klatki), więc
obiekty są jeszcze żywe po wołaniu — zaznaczenie i primary czyścimy od razu, żeby details panel
nie edytował przez tę klatkę trupa.

Gdyby multi-selection miał kiedyś działać w viewporcie/gizmo, trzeba go przenieść do
`EditorAppState` — wtedy **każde** miejsce ustawiające `SelectedGameObject` musi też
zaktualizować tablicę, inaczej wróci rozjazd, przed którym broni się `SyncSelection`.

## Kamera viewportu (jedna kamera, per-viewport stan)

W edytorze istnieje **jedna** `EditorSceneCamera` (`gEditorAppContext->EditorSceneCamera`), a każdy viewport pamięta tylko swój **widok** (lokacja, „nice" rotacja, `CameraOptions`) i dostaje go z powrotem przy wejściu — z punktu widzenia usera każdy viewport ma „swoją" kamerę.

Zakres jest celowo minimalny: **sam widok, nic o tym co się renderuje**. Viewport nie dotyka `SetEditorRenderCamera`, nie tworzy/nie kasuje overlaya poza swoim `OnOpened` i nie sprawdza, czy scena jest wczytana. Patrz „Znany problem: PIE" niżej — próby robienia tego stąd kończyły się regresjami.

- Nowy viewport nawigujący w 3D → nadpisz `IEditorViewport::UsesEditorCamera()` na `true`. Bez tego viewport nigdy nie przejmie kamery (i dobrze — viewporty typu Texture/Shader nie mają jej ruszać).
- Kamerę bierz przez `IEditorViewport::GetEditorCamera()`, **nie** twórz własnej instancji.
- Właściciela trzyma `EditorViewportManager` (`SetCameraOwner`/`GetCameraOwner`); przekazanie zapisuje stan poprzednika (`SaveCameraState`) i przywraca stan nowego (`ApplyCameraState`).
- Przejęcie dzieje się samo: na hoverze panelu (`IEditorPanel::BeginPanel`) i przy pojawieniu się viewportu (`IEditorViewport::BeginWindow`, po `OnOpened` paneli). Nie wołaj `ClaimEditorCamera()` ręcznie bez powodu.
- Pierwsze przejęcie **adoptuje** aktualny stan kamery zamiast go nadpisywać — dzięki temu pozycja wczytana ze sceny (`EditorCameraLocationLoaded`) czy startowy framing panelu stają się widokiem startowym viewportu.
- Dlatego framing (`NeedsFraming`) ustawiaj tylko raz (wartość początkowa pola) albo z przycisku „Frame" — **nie** w `OnOpened` panelu, bo skasowałby zapamiętany widok przy każdym powrocie do zakładki.

### Znany problem: PIE (nierozwiązany)

**W PIE viewporty meshy stoją** — `StaticMeshViewportPanel`/`SkeletalMeshViewportPanel` mają bramkę `if (!IsInPIE())` na sterowaniu kamerą, a „Frame" nie ma czego objąć. Nie jest to do naprawienia z poziomu viewportu i **nie próbuj tego łatać lokalnie** (było, wróciło regresjami). Przyczyny są architektoniczne:

- `SceneManager::EnterPIE()` woła `UnloadOverlayScene()`, a panel meshu odtwarza overlay tylko w `OnOpened`, czyli przy zmianie widoczności. Viewport widoczny w momencie Play zostaje bez swojego meshu do końca sesji PIE.
- Overlay to **singleton** (`mOverlayScene`), a `GetCurrentWorld()` = `overlay ?: PIE ?: active` — z czego korzysta **i tick** (`SceneManager::OnUpdate`) **i renderer** (`RenderSnapshotBuilder`). Odtworzenie overlaya w PIE zatrzymuje więc grę: nie tickuje i nie renderuje.
- Jest **jeden** główny FBO (`RequestMainFrameBuffer`) i **jeden** `RenderSnapshot` z jedną kamerą, więc PIE i podgląd meshu nie mogą renderować się równolegle.

Docelowo: wiele scen otwartych naraz + widok per viewport w snapshocie (własny FBO + własny świat + własna kamera). Uwaga na sprzężenie: **przy równoległych widokach każdy viewport potrzebuje żywej kamery co klatkę**, więc opisany wyżej model „jedna kamera + zapamiętany stan" przestaje wystarczać i zamienia się w realną kamerę per viewport (tak miały Skeleton/Animation przed ujednoliceniem). Skeleton/Animation działają w PIE, bo rysują ImGui draw listami i nie dotykają ścieżki renderu w ogóle.

## Zapis na żądanie (NIE zapisuj od razu)

Nie zapisuj assetu natychmiast po mutacji. Oznacz go brudnym i zostaw zapis ścieżce na żądanie (Ctrl+S w `BeginWindow`/`BeginPanel`, albo „Save All"). Wszystko idzie przez jeden punkt:

```
SaveAsset(assetDesc) → DispatchAssetSaveJSON / DispatchAssetSaveBinary
                     → mAssetLoaders[AssetType->TypeName]->DispatchAssetSave(...)
```

- Loadery (`IAssetLoader`) auto-rejestrują się po `GetSupportedAssetType()`. Nowy typ assetu z własnym zapisem → nadpisz `DispatchAssetSave` w jego loaderze (edytor-only). Przykłady: `SceneAssetHandler` (deleguje do `SceneManager::SaveActiveScene`), `StaticMeshAssetHandler` (woła `MeshImporter::SaveStaticMesh`).
- `IEditorViewport::mCanBeSaved` domyślnie = `LoaderType == JSON`. Dla assetów **binarnych** (np. StaticMesh) trzeba jawnie włączyć zapis: `SetCanBeSaved(true)` w `OnInit()`, inaczej Ctrl+S nie zadziała.

Powiązane: `EngineAssetManager` (`MarkAssetDirty`/`IsAssetDirty`/`AreAnyAssetsDirty`/`SaveAsset`), `IEditorViewport`, `IEditorPanel`.
