//
// Created by Plutex on 5/23/26.
//

#include "PluEngine/Core/Reflection/TypeTraits.h"

#include "PluEngine/AssetCore/AssetDescriptor.h"

#ifdef PLU_ENGINE_EDITOR_BUILD

bool Plu::TUsePointerAssetUI(void *value, String name, TypeInfo *typeInfo)
{
    TUsePointer<EngineAssetManager> assetManager = TypeRegistry::GetInstance()->GetAssetManager();
    TUsePointer<EngineObjectManager> engineObjectManager = TypeRegistry::GetInstance()->GetObjectManager();

    static GameHashMap<String, DynamicArray<TUsePointer<AssetDescriptor>>> allAssetsPerField;
    String mapKey = name + typeInfo->TypeName + String::FromInt(reinterpret_cast<UInt64>(value));
    if (!allAssetsPerField.Contains(mapKey)) {
        allAssetsPerField[mapKey] = assetManager->GetAllAssetDescriptorsOfType(typeInfo);
    }
    TUsePointer<AssetDescriptor> selected = nullptr;
    for (auto i : allAssetsPerField[mapKey]) {
        if (!static_cast<TUsePointer<IAssetData>*>(value)->GetRaw()) continue;
        if (static_cast<TUsePointer<IAssetData>*>(value)->GetRaw()->Uuid == i->Uuid) {
            selected = i;
        }
    }

	String preview = "Nothing Selected!";
	if (selected)
	{
		preview = selected->AssetName;
	}
	if (ImGui::BeginCombo(name.CStr(), preview.CStr(), 0))
    {
        // Refresh lives inside the dropdown to keep the row uncluttered.
        if (ImGui::SmallButton("Refresh"))
        {
            allAssetsPerField[mapKey] = assetManager->GetAllAssetDescriptorsOfType(typeInfo);
        }
        ImGui::SameLine();

        static ImGuiTextFilter filter;
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
            filter.Clear();
        }
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
        filter.Draw("##Filter", -FLT_MIN);

		bool is_selected = selected == nullptr;

		bool changed = false;

		if (ImGui::Selectable("NULL", is_selected)) {
		    *static_cast<TUsePointer<IAssetData>*>(value) = nullptr;
		    changed = true;
		}


        for (int n = 0; n < allAssetsPerField[mapKey].Size(); n++)
        {
            TUsePointer<AssetDescriptor> asset = allAssetsPerField[mapKey][n];
            PropertyInfo* nameProp = asset->GetClass()->FindProperty("Name");
            String objName;
            if (nameProp) {
                String* namePtr = static_cast<String *>(nameProp->GetPtr(asset.GetRaw()));
                objName = *namePtr;
            } else {
                objName = asset->AssetName;
            }
            is_selected = (asset == selected);
            if (filter.PassFilter(objName.CStr()))
                if (ImGui::Selectable(objName.CStr(), is_selected)) {
                    selected = asset;
                    *static_cast<TUsePointer<IAssetData>*>(value) = assetManager->GetAssetData(selected);
                    changed = true;
                }
        }
        ImGui::EndCombo();
		return changed;
    }
    return false;
}

bool Plu::UUIDForAssetUI(void* value, String name, TypeInfo* typeInfo, PropertyInfo* propertyInfo)
{
    static GameHashMap<String, DynamicArray<EngineObjectHandle>> objectsPerUuidField;
    static GameHashMap<String, DynamicArray<TUsePointer<AssetDescriptor>>> assetsPerUuidField;

    TUsePointer<EngineAssetManager> assetManager = TypeRegistry::GetInstance()->GetAssetManager();
    TUsePointer<EngineObjectManager> engineObjectManager = TypeRegistry::GetInstance()->GetObjectManager();

    bool changed = false;

    const bool isAsset = propertyInfo->UuidForClass->IsDerivedOfOrSame(IAssetData::GetStaticClass());

    // Cache the candidate list per target class, not per property name: two
    // unrelated properties that happen to share a name must not collide.
    const String cacheKey = propertyInfo->UuidForClass->TypeName;

    if (isAsset) {
        if (!assetsPerUuidField.Contains(cacheKey)) {
            assetsPerUuidField[cacheKey] = assetManager->GetAllAssetDescriptorsOfType(propertyInfo->UuidForClass);
        }
    } else {
        if (!objectsPerUuidField.Contains(cacheKey)) {
            objectsPerUuidField[cacheKey] = engineObjectManager->GetAllObjectsOfClass(propertyInfo->UuidForClass);
        }
    }

    // The UUID currently stored in the property is the single source of truth
    // for what is selected — never an ephemeral per-frame UI index.
    const UInt64 currentUuid = static_cast<PluUUID*>(value)->getUUID();

    if (isAsset) {
        DynamicArray<TUsePointer<AssetDescriptor>>& assets = assetsPerUuidField[cacheKey];

        String preview = "Nothing selected!";
        for (int n = 0; n < assets.Size(); n++) {
            if (assets[n] && assets[n]->Uuid == currentUuid) {
                preview = assets[n]->AssetName;
                break;
            }
        }

        if (ImGui::BeginCombo(propertyInfo->PropertyName.CStr(), preview.CStr(), 0))
        {
            // Refresh lives inside the dropdown to keep the row uncluttered.
            if (ImGui::SmallButton("Refresh")) {
                assets = assetManager->GetAllAssetDescriptorsOfType(propertyInfo->UuidForClass);
            }
            ImGui::SameLine();

            static ImGuiTextFilter filter;
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                filter.Clear();
            }
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
            filter.Draw("##Filter", -FLT_MIN);

            for (int n = 0; n < assets.Size(); n++)
            {
                TUsePointer<AssetDescriptor> asset = assets[n];
                if (!asset) continue;
                String objName = asset->AssetName;
                const bool is_selected = (asset->Uuid == currentUuid);
                if (filter.PassFilter(objName.CStr()))
                    if (ImGui::Selectable(objName.CStr(), is_selected)) {
                        *static_cast<PluUUID*>(value) = asset->Uuid;
                        changed = true;
                    }
            }
            ImGui::EndCombo();
        }
    } else {
        DynamicArray<EngineObjectHandle>& objects = objectsPerUuidField[cacheKey];

        String preview = "Nothing selected!";
        for (int n = 0; n < objects.Size(); n++) {
            TUsePointer<EngineObject> obj = engineObjectManager->GetObjectAsUser<EngineObject>(objects[n]);
            if (!obj) continue;
            void* uuidPropPtr = obj->GetClass()->GetTypeUuidProp()->GetPtr(obj.GetRaw());
            if (static_cast<PluUUID*>(uuidPropPtr)->getUUID() == currentUuid) {
                PropertyInfo* nameProp = obj->GetClass()->FindProperty("Name");
                preview = nameProp ? *static_cast<String*>(nameProp->GetPtr(obj.GetRaw())) : obj->GetClass()->TypeName;
                break;
            }
        }

        if (ImGui::BeginCombo(propertyInfo->PropertyName.CStr(), preview.CStr(), 0))
        {
            // Refresh lives inside the dropdown to keep the row uncluttered.
            if (ImGui::SmallButton("Refresh")) {
                objects = engineObjectManager->GetAllObjectsOfClass(propertyInfo->UuidForClass);
            }
            ImGui::SameLine();

            static ImGuiTextFilter filter;
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                filter.Clear();
            }
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
            filter.Draw("##Filter", -FLT_MIN);

            for (int n = 0; n < objects.Size(); n++)
            {
                TUsePointer<EngineObject> obj = engineObjectManager->GetObjectAsUser<EngineObject>(objects[n]);
                if (!obj) continue;
                PropertyInfo* nameProp = obj->GetClass()->FindProperty("Name");
                String objName = nameProp ? *static_cast<String*>(nameProp->GetPtr(obj.GetRaw())) : obj->GetClass()->TypeName;

                void* uuidPropPtr = obj->GetClass()->GetTypeUuidProp()->GetPtr(obj.GetRaw());
                const UInt64 objUuid = static_cast<PluUUID*>(uuidPropPtr)->getUUID();
                const bool is_selected = (objUuid == currentUuid);
                if (filter.PassFilter(objName.CStr()))
                    if (ImGui::Selectable(objName.CStr(), is_selected)) {
                        *static_cast<PluUUID*>(value) = objUuid;
                        changed = true;
                    }
            }
            ImGui::EndCombo();
        }
    }
    return changed;
}

bool Plu::ArrayTreeEditorControl(void* arrayId, const String& name, UInt64 count,
                                  const std::function<bool(UInt64 index)>& renderElement,
                                  const std::function<void()>& addElement,
                                  const std::function<void(UInt64 index)>& removeElement)
{
    // Kubełki po 10: płaski TreeNode na element skaluje się fatalnie przy setkach/tysiącach
    // wpisów (foliage, InstancedStaticMeshComponent::Instances) - ImGui ma wtedy tysiące węzłów
    // do zrenderowania i panel detali staje się nieprzewijalną ścianą. Grupowanie po 10 trzyma
    // widoczną liczbę węzłów w ryzach, dopóki nie ma dedykowanego widgetu z wirtualizacją.
    constexpr UInt64 kChunkSize = 10;

    ImGui::PushID(arrayId);
    bool changed = false;

    // TreeNodeEx z osobnym str_id: label pojedynczo-argumentowego ImGui::TreeNode(text) JEST jego
    // ID, więc gdyby tekst zawierał "count" (zmienia się po Add/Remove), ImGui widziałby to jako
    // zupełnie inny węzeł co klatkę i zwijałby go z powrotem do stanu domyślnego (closed) - stąd
    // "Add zamyka TreeNode". str_id musi być stały; dynamiczny tekst idzie tylko przez fmt.
    if (ImGui::TreeNodeEx("##ArrayRoot", ImGuiTreeNodeFlags_None, "%s (%llu)", name.CStr(), static_cast<unsigned long long>(count)))
    {
        if (ImGui::SmallButton("+ Add")) {
            addElement();
            changed = true;
        }

        // Usuwanie odłożone do końca pętli: renderElement/removeElement mutowałyby kontener,
        // po którym ta pętla iteruje po indeksie - usunięcie w środku przesunęłoby indeksy
        // pozostałych elementów pod stopami tej samej klatki.
        Int64 pendingRemove = -1;

        for (UInt64 chunkStart = 0; chunkStart < count; chunkStart += kChunkSize) {
            const UInt64 chunkEnd = (chunkStart + kChunkSize <= count) ? (chunkStart + kChunkSize - 1) : (count - 1);

            ImGui::PushID(static_cast<int>(chunkStart));
            // Ten sam powód co przy "##ArrayRoot": chunkEnd zmienia się w ostatnim (niepełnym)
            // kubełku przy Add/Remove, więc str_id musi być stały - "##Chunk" plus już wypchnięty
            // chunkStart w ID-stacku wystarczą do unikalności między kubełkami.
            if (ImGui::TreeNodeEx("##Chunk", ImGuiTreeNodeFlags_None, "Elements %llu-%llu",
                                   static_cast<unsigned long long>(chunkStart), static_cast<unsigned long long>(chunkEnd)))
            {
                for (UInt64 index = chunkStart; index <= chunkEnd; index++) {
                    ImGui::PushID(static_cast<int>(index));
                    if (ImGui::SmallButton("X")) {
                        pendingRemove = static_cast<Int64>(index);
                    }
                    ImGui::SameLine();
                    if (renderElement(index)) {
                        changed = true;
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        if (pendingRemove >= 0) {
            removeElement(static_cast<UInt64>(pendingRemove));
            changed = true;
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
    return changed;
}

bool Plu::CollisionProfileRefEditorControl(void* value, const String& name)
{
    auto* ref = static_cast<CollisionProfileRef*>(value);
    const CollisionConfig& cfg = ActiveCollisionConfig();

    bool changed = false;
    if (ImGui::BeginCombo(name.CStr(), ref->Name.CStr()))
    {
        for (UInt32 i = 0; i < cfg.Profiles.Size(); ++i)
        {
            const bool selected = (cfg.Profiles[i].Name == ref->Name);
            if (ImGui::Selectable(cfg.Profiles[i].Name.CStr(), selected))
            {
                ref->Name = cfg.Profiles[i].Name;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

#endif
