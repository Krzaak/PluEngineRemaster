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

## Zapis na żądanie (NIE zapisuj od razu)

Nie zapisuj assetu natychmiast po mutacji. Oznacz go brudnym i zostaw zapis ścieżce na żądanie (Ctrl+S w `BeginWindow`/`BeginPanel`, albo „Save All"). Wszystko idzie przez jeden punkt:

```
SaveAsset(assetDesc) → DispatchAssetSaveJSON / DispatchAssetSaveBinary
                     → mAssetLoaders[AssetType->TypeName]->DispatchAssetSave(...)
```

- Loadery (`IAssetLoader`) auto-rejestrują się po `GetSupportedAssetType()`. Nowy typ assetu z własnym zapisem → nadpisz `DispatchAssetSave` w jego loaderze (edytor-only). Przykłady: `SceneAssetHandler` (deleguje do `SceneManager::SaveActiveScene`), `StaticMeshAssetHandler` (woła `MeshImporter::SaveStaticMesh`).
- `IEditorViewport::mCanBeSaved` domyślnie = `LoaderType == JSON`. Dla assetów **binarnych** (np. StaticMesh) trzeba jawnie włączyć zapis: `SetCanBeSaved(true)` w `OnInit()`, inaczej Ctrl+S nie zadziała.

Powiązane: `EngineAssetManager` (`MarkAssetDirty`/`IsAssetDirty`/`AreAnyAssetsDirty`/`SaveAsset`), `IEditorViewport`, `IEditorPanel`.
