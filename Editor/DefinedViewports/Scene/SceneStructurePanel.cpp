//
// Created by Plutex on 1/14/26.
//

#include "SceneStructurePanel.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Application.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"
#include "UI/IconsFontAwesome7.h"
#include "Utils/RandomTransformUtils.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::ApplicationInfo* gApplicationInfo;

namespace
{
	// (Prefiks w nazwach: edytor to UNITY_BUILD.)

	/** Długość nazwy po odcięciu końcowych cyfr: "Tree12" -> 4. Samych cyfr nie tnie. */
	UInt64 SceneStructureNameBaseLength(const Plu::String& name)
	{
		UInt64 end = name.Length();
		while (end > 0 && name[end - 1] >= '0' && name[end - 1] <= '9') {
			--end;
		}
		return end == 0 ? name.Length() : end;
	}

	/** Nazwa bez końcowych cyfr: "Tree12" -> "Tree". */
	Plu::String SceneStructureNameBase(const Plu::String& name)
	{
		return name.Substring(0, SceneStructureNameBaseLength(name));
	}

	/** Numerek z końca nazwy albo -1, gdy go nie ma ("Tree12" -> 12, "Tree" -> -1). */
	Int64 SceneStructureNameNumber(const Plu::String& name)
	{
		const UInt64 base = SceneStructureNameBaseLength(name);
		if (base >= name.Length()) {
			return -1;
		}
		bool         parsed = false;
		const UInt32 value  = name.Substring(base).ToInt<UInt32>(&parsed);
		return parsed ? static_cast<Int64>(value) : -1;
	}

	// Nazwa jest PLU_PROPERTY, więc jedzie w JSON-ie razem z resztą pól — klon dostałby
	// nazwę oryginału. Podmieniamy ją na kolejny wolny numerek *tego samego prefiksu*,
	// żeby duplikat "Tree3" nazwał się "Tree4", a nie domyślnym "StaticMeshActor7".
	// Liczyć trzeba przed każdym spawnem osobno (Duplicate N times), bo poprzedni klon
	// zajmuje już swój numerek.
	void SceneStructureRenameClone(JSON& j, const Plu::TUsePointer<Plu::SceneWorld>& world)
	{
		if (!j.contains("fields") || !world) {
			return;
		}
		for (auto it = j["fields"].begin(); it != j["fields"].end(); ++it) {
			if (!it->contains("name") || (*it)["name"] != "mObjectName") {
				continue;
			}
			// Brak nazwy (obiekt spoza SpawnGameObject) — pole out, SpawnGameObject nada domyślną.
			const Plu::String current((*it)["value"].get<std::string>().c_str());
			if (current.IsEmpty()) {
				j["fields"].erase(it);
				return;
			}
			(*it)["value"] = world->MakeDefaultObjectNameFromBase(SceneStructureNameBase(current)).CStr();
			return;
		}
	}

	// Prefiks trzymamy jako wskaźnik w oryginalną nazwę + długość, żeby sortowanie (co klatkę!)
	// nie alokowało jednego Stringa na obiekt. Tablica nazw nie zmienia się w trakcie sortowania.
	struct SceneStructureSortEntry
	{
		const char* Base    = nullptr;
		UInt64      BaseLen = 0;
		Int64       Number  = -1;
		UInt64      Index   = 0;
	};

	// Kolejność listy: grupy po prefiksie nazwy (alfabetycznie), a wewnątrz grupy malejąco po
	// numerku — najwyższe numery na górze, "...0" (i nazwy bez numerka) na dole.
	// Klucze parsujemy raz do tablicy pomocniczej; samo porównanie w sortowaniu jest wtedy tanie.
	void SceneStructureSortByName(DynamicArray<Plu::TUsePointer<Plu::GameObject>>* objects,
	                              DynamicArray<Plu::String>*                      names)
	{
		PLU_PROFILE_SCOPE("SceneStructurePanel::SortByName");

		const UInt64 count = names->Size() < objects->Size() ? names->Size() : objects->Size();

		DynamicArray<SceneStructureSortEntry> order;
		order.Reserve(count);
		for (UInt64 i = 0; i < count; ++i) {
			SceneStructureSortEntry entry;
			entry.Base    = names->At(i).CStr();
			entry.BaseLen = SceneStructureNameBaseLength(names->At(i));
			entry.Number  = SceneStructureNameNumber(names->At(i));
			entry.Index   = i;
			order.PushBack(entry);
		}

		order.Sort([](const SceneStructureSortEntry& a, const SceneStructureSortEntry& b) -> bool {
			const UInt64 shared = a.BaseLen < b.BaseLen ? a.BaseLen : b.BaseLen;
			const int    cmp    = shared > 0 ? memcmp(a.Base, b.Base, shared) : 0;
			if (cmp != 0) {
				return cmp < 0;
			}
			// Wspólny początek — krótszy prefiks pierwszy ("Tree" przed "TreeBig").
			if (a.BaseLen != b.BaseLen) {
				return a.BaseLen < b.BaseLen;
			}
			if (a.Number != b.Number) {
				return a.Number > b.Number;
			}
			// Indeks jako rozstrzygacz — bez tego obiekty o identycznej nazwie skakałyby po liście.
			return a.Index < b.Index;
		});

		DynamicArray<Plu::TUsePointer<Plu::GameObject>> sortedObjects;
		DynamicArray<Plu::String>                      sortedNames;
		sortedObjects.Reserve(count);
		sortedNames.Reserve(count);
		for (UInt64 i = 0; i < count; ++i) {
			sortedObjects.PushBack(objects->At(order.At(i).Index));
			sortedNames.PushBack(names->At(order.At(i).Index));
		}
		*objects = sortedObjects;
		*names   = sortedNames;
	}
}

Plu::SceneStructurePanel::~SceneStructurePanel()
{
	UnsubscribeFromWorld();
}

Plu::String Plu::SceneStructurePanel::GetPanelName()
{
	return "Structure";
}

void Plu::SceneStructurePanel::OnClosed()
{
	UnsubscribeFromWorld();
}

void Plu::SceneStructurePanel::OnOpened()
{
	// Panel mógł być zamknięty w czasie, gdy scena się zmieniła.
	mListDirty = true;
}

void Plu::SceneStructurePanel::UnsubscribeFromWorld()
{
	if (mSubscribedWorld && mGameObjectsChangedHandle != 0) {
		mSubscribedWorld->GetObjectEventDispatcher()->Unsubscribe("GameObjectsChanged", mGameObjectsChangedHandle);
	}
	mSubscribedWorld          = nullptr;
	mGameObjectsChangedHandle = 0;
}

void Plu::SceneStructurePanel::EnsureSubscribedTo(const TUsePointer<SceneWorld>& world)
{
	// GetCurrentWorld() to overlay/PIE/aktywna scena — świat potrafi się podmienić pod panelem,
	// a subskrypcja wisi na konkretnym obiekcie, więc trzeba ją wtedy przepiąć.
	if (mSubscribedWorld && world && *mSubscribedWorld->GetEngineObjectHandle() == *world->GetEngineObjectHandle()) {
		return;
	}

	UnsubscribeFromWorld();
	if (!world) {
		mListDirty = true;
		return;
	}

	mSubscribedWorld          = world;
	mGameObjectsChangedHandle = world->GetObjectEventDispatcher()->Subscribe("GameObjectsChanged", [this](void*) {
		mListDirty = true;
	});
	mListDirty = true;
}

void Plu::SceneStructurePanel::RefreshObjectList(const TUsePointer<SceneWorld>& world)
{
	PLU_PROFILE_SCOPE("SceneStructurePanel::RefreshObjectList");

	mListObjects = world->GetAllGameObjects();
	world->GetFormattedGameObjectNames(&mListNames);
	// Obie tablice idą w tej samej kolejności (jeden przebieg po mGameObjects), więc
	// sortujemy je razem — indeksy wierszy (kotwica zaznaczenia, Shift+klik) muszą
	// odpowiadać temu, co user widzi.
	SceneStructureSortByName(&mListObjects, &mListNames);
	mListDirty = false;
}

Int64 Plu::SceneStructurePanel::FindInSelection(const EngineObjectHandle& handle) const
{
	for (UInt64 i = 0; i < mSelectedObjects.Size(); ++i) {
		if (mSelectedObjects.At(i) == handle) {
			return static_cast<Int64>(i);
		}
	}
	return -1;
}

bool Plu::SceneStructurePanel::IsSelected(const EngineObjectHandle& handle) const
{
	return FindInSelection(handle) >= 0;
}

void Plu::SceneStructurePanel::SetPrimarySelection(const EngineObjectHandle& handle)
{
	gEditorAppContext->EditorState.SelectedGameObject          = handle;
	gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
}

void Plu::SceneStructurePanel::SyncSelection(const DynamicArray<TUsePointer<GameObject>>& objects)
{
	PLU_PROFILE_SCOPE("SceneStructurePanel::SyncSelection");

	// Obiekt kasowany w trakcie zmiany nazwy zostawiłby panel w trybie edycji na zawsze.
	if (mRenamingObject.failed == false && !gEngineObjectManager->IsValid(mRenamingObject)) {
		mRenamingObject     = EngineObjectHandle();
		mRenameFocusPending = false;
	}

	// Wyrzuć obiekty, których już nie ma (usunięte Deletem, przeładowana scena).
	for (Int64 i = static_cast<Int64>(mSelectedObjects.Size()) - 1; i >= 0; --i) {
		if (!gEngineObjectManager->IsValid(mSelectedObjects.At(static_cast<UInt64>(i)))) {
			mSelectedObjects.RemoveAt(static_cast<UInt64>(i));
		}
	}

	// Zaznaczenie zrobione poza panelem (klik w viewporcie, Delete) jest źródłem prawdy —
	// inaczej panel pokazywałby podświetlenie rozjechane z tym, co edytuje details panel.
	const EngineObjectHandle& primary = gEditorAppContext->EditorState.SelectedGameObject;
	if (!gEngineObjectManager->IsValid(primary)) {
		mSelectedObjects.Clear();
		mSelectionAnchor = -1;
		return;
	}

	if (!IsSelected(primary)) {
		mSelectedObjects.Clear();
		mSelectedObjects.PushBack(primary);
		mSelectionAnchor = -1;
		for (UInt64 i = 0; i < objects.Size(); ++i) {
			if (*objects.At(i)->GetEngineObjectHandle() == primary) {
				mSelectionAnchor = static_cast<Int64>(i);
				break;
			}
		}
	}
}

void Plu::SceneStructurePanel::SelectSingle(const EngineObjectHandle& handle, Int64 index)
{
	mSelectedObjects.Clear();
	mSelectedObjects.PushBack(handle);
	mSelectionAnchor = index;
	SetPrimarySelection(handle);
}

void Plu::SceneStructurePanel::ToggleSelection(const EngineObjectHandle& handle, Int64 index)
{
	const Int64 found = FindInSelection(handle);
	if (found >= 0) {
		mSelectedObjects.RemoveAt(static_cast<UInt64>(found));
		// Primary musi zostać w zaznaczeniu, inaczej SyncSelection wciągnie go z powrotem.
		if (gEditorAppContext->EditorState.SelectedGameObject == handle) {
			SetPrimarySelection(mSelectedObjects.IsEmpty() ? EngineObjectHandle()
			                                               : mSelectedObjects.At(mSelectedObjects.Size() - 1));
		}
		return;
	}

	mSelectedObjects.PushBack(handle);
	mSelectionAnchor = index;
	SetPrimarySelection(handle);
}

void Plu::SceneStructurePanel::SelectRangeTo(const DynamicArray<TUsePointer<GameObject>>& objects, Int64 index)
{
	if (mSelectionAnchor < 0 || mSelectionAnchor >= static_cast<Int64>(objects.Size())) {
		SelectSingle(*objects.At(static_cast<UInt64>(index))->GetEngineObjectHandle(), index);
		return;
	}

	const Int64 from = mSelectionAnchor < index ? mSelectionAnchor : index;
	const Int64 to   = mSelectionAnchor < index ? index : mSelectionAnchor;

	mSelectedObjects.Clear();
	for (Int64 i = from; i <= to; ++i) {
		mSelectedObjects.PushBack(*objects.At(static_cast<UInt64>(i))->GetEngineObjectHandle());
	}
	// Anchor zostaje, żeby kolejny shift+klik rozciągał ten sam zakres.
	SetPrimarySelection(*objects.At(static_cast<UInt64>(index))->GetEngineObjectHandle());
}

void Plu::SceneStructurePanel::BeginRename(const EngineObjectHandle& handle, const String& currentName)
{
	mRenamingObject     = handle;
	mRenameFocusPending = true;
	const UInt64 length = currentName.Length() < sizeof(mRenameBuffer) - 1 ? currentName.Length()
	                                                                      : sizeof(mRenameBuffer) - 1;
	memcpy(mRenameBuffer, currentName.CStr(), length);
	mRenameBuffer[length] = '\0';
}

void Plu::SceneStructurePanel::CommitRename(const TUsePointer<GameObject>& object)
{
	const String newName(mRenameBuffer);
	if (object && !newName.IsEmpty() && newName != object->GetObjectName()) {
		object->SetObjectName(newName);
		PanelChangedAsset();
		// Nazwa jest kluczem sortowania, więc lista musi się przebudować.
		mListDirty = true;
	}
	mRenamingObject     = EngineObjectHandle();
	mRenameFocusPending = false;
}

void Plu::SceneStructurePanel::DeleteSelectedObjects()
{
	PLU_PROFILE_SCOPE("SceneStructurePanel::DeleteSelectedObjects");

	if (mSelectedObjects.IsEmpty()) {
		return;
	}
	TUsePointer<SceneWorld> world = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
	if (!world) {
		return;
	}

	for (UInt64 i = 0; i < mSelectedObjects.Size(); ++i) {
		if (!gEngineObjectManager->IsValid(mSelectedObjects.At(i))) {
			continue;
		}
		world->DeleteGameObject(mSelectedObjects.At(i));
	}

	// Kasowanie jest odroczone (mObjectsToDestroy zbiera się do końca klatki), więc obiekty
	// są jeszcze żywe — zaznaczenie czyścimy od razu, żeby details panel nie edytował trupa.
	mSelectedObjects.Clear();
	mSelectionAnchor = -1;
	SetPrimarySelection(EngineObjectHandle());
	// Event "GameObjectsChanged" przyjdzie dopiero po faktycznym skasowaniu, ale lista i tak
	// musi zejść z ekranu w tej klatce.
	mListDirty = true;
}

void Plu::SceneStructurePanel::ApplyRandomTransforms()
{
	PLU_PROFILE_SCOPE("SceneStructurePanel::ApplyRandomTransforms");

	const UInt32 count = static_cast<UInt32>(mSelectedObjects.Size());
	if (count == 0) {
		return;
	}

	const RandomizeTransformsSettings& s = mRandomizeSettings;

	// Ten sam seed dla trzech kanałów dałby skorelowane wyniki (ta sama sekwencja
	// znormalizowanych wartości), więc rozjeżdżamy je stałymi. Przy seedzie "auto"
	// NIE wolno tego robić — RandomTransformSeedAuto (0) to wartownik, każde xor
	// zamieniłoby losowanie w deterministyczne.
	const UInt64 baseSeed  = s.UseFixedSeed ? static_cast<UInt64>(s.Seed) : RandomTransformSeedAuto;
	const bool   isAuto    = (baseSeed == RandomTransformSeedAuto);
	const UInt64 locSeed   = baseSeed;
	const UInt64 rotSeed   = isAuto ? RandomTransformSeedAuto : (baseSeed ^ 0x9E3779B97F4A7C15ull);
	const UInt64 scaleSeed = isAuto ? RandomTransformSeedAuto : (baseSeed ^ 0xD1B54A32D192ED03ull);

	DynamicArray<Vec3> locations;
	DynamicArray<Vec3> rotations;
	DynamicArray<Vec3> scales;

	if (s.RandomizeLocation) {
		GenerateRandomLocations(&locations, count, s.LocationMin, s.LocationMax, locSeed);
	}
	if (s.RandomizeRotation) {
		GenerateRandomRotations(&rotations, count, s.RotationMin, s.RotationMax, rotSeed);
	}
	if (s.RandomizeScale) {
		if (s.UniformScale) {
			GenerateRandomUniformScales(&scales, count, s.UniformScaleMin, s.UniformScaleMax, scaleSeed);
		} else {
			GenerateRandomScales(&scales, count, s.ScaleMin, s.ScaleMax, scaleSeed);
		}
	}

	for (UInt64 i = 0; i < mSelectedObjects.Size(); ++i) {
		if (!gEngineObjectManager->IsValid(mSelectedObjects.At(i))) {
			continue;
		}
		TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(mSelectedObjects.At(i));
		if (!obj) {
			continue;
		}

		if (s.RandomizeLocation) {
			obj->SetObjectLocation(s.LocationRelative ? obj->GetObjectLocation() + locations.At(i)
			                                          : locations.At(i));
		}
		if (s.RandomizeRotation) {
			obj->SetObjectRotation(rotations.At(i));
		}
		if (s.RandomizeScale) {
			obj->SetObjectScale(scales.At(i));
		}
	}

	PanelChangedAsset();
}

void Plu::SceneStructurePanel::DrawRandomizeWindow()
{
	if (!mShowRandomizeWindow) {
		return;
	}

	// Obwódka jak w niezadokowanych panelach (IEditorPanel::BeginPanel). Stan dokowania
	// znamy dopiero po Begin(), więc pushujemy na podstawie poprzedniej klatki —
	// jedna klatka opóźnienia na obwódce jest niewidoczna.
	const bool pushBorder = !mRandomizeWindowDocked;
	if (pushBorder) {
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	}

	const String title = String("Randomize Transforms##") + GetParentViewport()->GetWindowName();
	// Begin/End muszą być sparowane nawet gdy okno jest zwinięte.
	const bool visible = ImGui::Begin(title.CStr(), &mShowRandomizeWindow, ImGuiWindowFlags_AlwaysAutoResize);
	mRandomizeWindowDocked = ImGui::IsWindowDocked();

	if (!visible) {
		ImGui::End();
		if (pushBorder) {
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
		return;
	}

	RandomizeTransformsSettings& s = mRandomizeSettings;

	ImGui::Text("Selected objects: %llu", static_cast<unsigned long long>(mSelectedObjects.Size()));
	ImGui::Separator();

	ImGui::Checkbox("Location", &s.RandomizeLocation);
	if (s.RandomizeLocation) {
		ImGui::Indent();
		ImGui::DragFloat3("Min##loc", glm::value_ptr(s.LocationMin), 0.1f);
		ImGui::DragFloat3("Max##loc", glm::value_ptr(s.LocationMax), 0.1f);
		ImGui::Checkbox("Relative to current location", &s.LocationRelative);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("On: the random value is an offset added to the current position.\n"
			                  "Off: objects land inside the given box.");
		}
		ImGui::Unindent();
	}

	ImGui::Separator();
	ImGui::Checkbox("Rotation", &s.RandomizeRotation);
	if (s.RandomizeRotation) {
		ImGui::Indent();
		ImGui::DragFloat3("Min##rot", glm::value_ptr(s.RotationMin), 1.0f);
		ImGui::DragFloat3("Max##rot", glm::value_ptr(s.RotationMax), 1.0f);
		ImGui::TextDisabled("Degrees: pitch=X, yaw=Y, roll=Z");
		ImGui::Unindent();
	}

	ImGui::Separator();
	ImGui::Checkbox("Scale", &s.RandomizeScale);
	if (s.RandomizeScale) {
		ImGui::Indent();
		ImGui::Checkbox("Uniform", &s.UniformScale);
		if (s.UniformScale) {
			ImGui::DragFloat("Min##uscale", &s.UniformScaleMin, 0.01f);
			ImGui::DragFloat("Max##uscale", &s.UniformScaleMax, 0.01f);
		} else {
			ImGui::DragFloat3("Min##scale", glm::value_ptr(s.ScaleMin), 0.01f);
			ImGui::DragFloat3("Max##scale", glm::value_ptr(s.ScaleMax), 0.01f);
		}
		ImGui::Unindent();
	}

	ImGui::Separator();
	ImGui::Checkbox("Fixed seed", &s.UseFixedSeed);
	if (s.UseFixedSeed) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragInt("##seed", &s.Seed);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Same seed + same ranges = same layout every time.");
		}
	} else {
		ImGui::SameLine();
		ImGui::TextDisabled("(different layout every time)");
	}

	ImGui::Separator();

	const bool nothingToDo = !s.RandomizeLocation && !s.RandomizeRotation && !s.RandomizeScale;
	ImGui::BeginDisabled(nothingToDo);
	if (ImGui::Button("Apply", ImVec2(120, 0))) {
		ApplyRandomTransforms();
		// Okno zostaje otwarte — przy losowym seedzie kolejne kliknięcia pozwalają
		// "przerzucać" układ aż do skutku.
	}
	ImGui::EndDisabled();
	if (nothingToDo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Enable at least one channel (location / rotation / scale).");
	}

	ImGui::SameLine();
	if (ImGui::Button("Close", ImVec2(120, 0))) {
		mShowRandomizeWindow = false;
	}

	ImGui::End();
	if (pushBorder) {
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}

void Plu::SceneStructurePanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<SceneInfo> scene = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen())
		{
			TUsePointer<SceneWorld> sceneWorld = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
			if (ImGui::BeginMenu(ICON_FA_PLUS " Spawn Game Object"))
			{
				static DynamicArray<TypeInfo*> componentTypes;
				if (componentTypes.IsEmpty()) {
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObject::GetStaticClass()) && !type.second->IsAbstract) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				if (ImGui::Button("Refresh")) {
					componentTypes.Clear();
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObject::GetStaticClass()) && !type.second->IsAbstract) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				ImGui::Separator();
				for (auto type : componentTypes) {
					if (ImGui::Button(type->TypeName.CStr())) {
						sceneWorld->SpawnGameObject(type);
					}
				}
				ImGui::EndMenu();
			}

			// Budowa listy (kopia tablicy obiektów + nazw + sortowanie) to przy tysiącu
			// obiektów ~1 ms, więc trzymamy ją między klatkami i odbudowujemy dopiero gdy
			// świat powie, że coś się zmieniło.
			EnsureSubscribedTo(sceneWorld);
			if (mListDirty) {
				RefreshObjectList(sceneWorld);
			}

			const DynamicArray<TUsePointer<GameObject>>& objects = mListObjects;
			const DynamicArray<String>&                  names   = mListNames;
			const UInt64 numObjs = names.Size() < objects.Size() ? names.Size() : objects.Size();

			SyncSelection(objects);

			if (mSelectedObjects.Size() > 1) {
				ImGui::Separator();
				if (ImGui::Button(ICON_FA_DICE " Randomize Transforms")) {
					mShowRandomizeWindow = true;
				}
				ImGui::SameLine();
				ImGui::TextDisabled("(%llu selected)", static_cast<unsigned long long>(mSelectedObjects.Size()));
			}
			ImGui::Separator();

			if (ImGui::Shortcut(ImGuiMod_Ctrl + ImGuiKey_D)) {
				if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
					TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
					JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&obj);
					SceneStructureRenameClone(j, sceneWorld);
					j["uuid"] = PluUUID().getUUID();
					gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
				}
			}

			for (UInt64 i = 0; i < numObjs; ++i) {
				TUsePointer<GameObject> object = objects.At(i);
				// Lista jest cache'em, więc obiekt mógł zniknąć bez eventu (ścieżka spoza
				// edytorskiego batcha) — wtedy odbudowa w następnej klatce.
				if (!object) {
					mListDirty = true;
					continue;
				}
				const EngineObjectHandle handle = *object->GetEngineObjectHandle();

				ImGui::PushID(static_cast<int>(i));

				if (mRenamingObject == handle) {
					if (mRenameFocusPending) {
						ImGui::SetKeyboardFocusHere();
						mRenameFocusPending = false;
					}
					ImGui::SetNextItemWidth(-FLT_MIN);
					const bool submitted = ImGui::InputText("##rename", mRenameBuffer, sizeof(mRenameBuffer),
					                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
					// IsItemDeactivated łapie i Esc, i klik poza polem. Esc przywraca starą
					// wartość do bufora, więc CommitRename i tak nic wtedy nie zapisze.
					if (submitted || ImGui::IsItemDeactivated()) {
						CommitRename(object);
					}
					ImGui::PopID();
					continue;
				}

				if (ImGui::Selectable(names[i].CStr(), IsSelected(handle), ImGuiSelectableFlags_AllowDoubleClick)) {
					const ImGuiIO& io = ImGui::GetIO();
					if (io.KeyCtrl) {
						ToggleSelection(handle, static_cast<Int64>(i));
					} else if (io.KeyShift) {
						SelectRangeTo(objects, static_cast<Int64>(i));
					} else {
						SelectSingle(handle, static_cast<Int64>(i));
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
							BeginRename(handle, names[i]);
						}
					}
				}

				if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
				{
					// Prawy klik na czymś spoza zaznaczenia przestawia je na ten obiekt;
					// wewnątrz zaznaczenia zostawia je w spokoju (inaczej nie dałoby się
					// otworzyć menu dla wielu obiektów naraz).
					if (!IsSelected(handle)) {
						SelectSingle(handle, static_cast<Int64>(i));
					} else {
						SetPrimarySelection(handle);
					}

					if (mSelectedObjects.Size() > 1) {
						if (ImGui::Button(ICON_FA_DICE " Randomize Transforms")) {
							mShowRandomizeWindow = true;
							ImGui::CloseCurrentPopup();
						}
						ImGui::Separator();
					}

					if (ImGui::Button("Rename")) {
						BeginRename(handle, names[i]);
						ImGui::CloseCurrentPopup();
					}

					// Klawisz Delete obsługuje SceneViewport (dla całego okna), tu tylko pozycja w menu.
					if (ImGui::Button(mSelectedObjects.Size() > 1 ? ICON_FA_TRASH " Delete selected"
					                                             : ICON_FA_TRASH " Delete")) {
						DeleteSelectedObjects();
						ImGui::CloseCurrentPopup();
					}
					ImGui::Separator();

					ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_D);
					if (ImGui::Button("Duplicate")) {
						JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&object);
						SceneStructureRenameClone(j, sceneWorld);
						j["uuid"] = PluUUID().getUUID();
						gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
						ImGui::CloseCurrentPopup();
					}
					static int numTimesToDupe = 1;
					if (ImGui::Button("Duplicate N times")) {
						JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&object);
						for (int n = 0; n < numTimesToDupe; ++n) {
							// Nazwę liczymy co iterację — poprzedni klon zajął już swój numerek.
							SceneStructureRenameClone(j, sceneWorld);
							j["uuid"] = PluUUID().getUUID();
							gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					ImGui::DragInt("##NtimeToDupe", &numTimesToDupe);
					if (ImGui::Button("Fit In View")) {
						TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
						BoundingBox boundingBox = {{-0.1,0.1},{-0.1,0.1},{-0.1,0.1}};
						for (const auto& comp : *obj->GetObjectWorldComponents()) {
							if (comp->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())) {
								BoundingBox newBox = CreateBoundingBoxForStaticMesh(DynamicCast<StaticMeshComponent>(comp)->GetStaticMesh().GetRaw());
								newBox = newBox.Multiply(DynamicCast<WorldComponent>(comp)->GetWorldScale());
								boundingBox = boundingBox.Add(newBox);
							}
						}
						Vec3 newLoc = boundingBox.FitCamera(obj->GetObjectLocation(),
							gEditorAppContext->EditorSceneCamera->GetCameraRotation(),
							Vec2(gApplicationInfo->AppWindow->GetWidth(), gApplicationInfo->AppWindow->GetHeight()),
							gEditorAppContext->EditorSceneCamera->GetCameraOptions()->FieldOfView
							);
						DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)->SetCameraLocation(newLoc);
					}
					ImGui::Separator();
					if (ImGui::Button("Close"))
						ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
				ImGui::PopID();
			}

		}
	}
	EndPanel();

	// Poza BeginPanel/EndPanel: to samodzielne okno, nie zawartość panelu — inaczej
	// dziedziczyłoby kontekst stylu panelu i znikało razem z jego zakładką.
	DrawRandomizeWindow();
}
