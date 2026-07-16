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
