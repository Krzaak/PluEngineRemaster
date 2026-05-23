//
// Created by Plutex on 5/23/26.
//

#include "PluEngine/Reflection/TypeTraits.h"

#include "PluEngine/Assets/AssetDescriptor.h"

#ifdef PLU_ENGINE_EDITOR_BUILD

bool Plu::TUsePointerAssetUI(void *value, String name, TypeInfo *typeInfo)
{
    TUsePointer<EngineAssetManager> assetManager = TypeRegistry::GetInstance()->GetAssetManager();
    TUsePointer<EngineObjectManager> engineObjectManager = TypeRegistry::GetInstance()->GetObjectManager();

    static GameHashMap<String, DynamicArray<TUsePointer<AssetDescriptor>>> allAssetsPerField;
    String mapKey = name + typeInfo->TypeName + reinterpret_cast<const char *>(value);
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

    if (ImGui::Button(("Refresh##" + name).CStr()))
	{
        allAssetsPerField[mapKey] = assetManager->GetAllAssetDescriptorsOfType(typeInfo);
	}
	String preview = "Nothing Selected!";
	if (selected)
	{
		preview = selected->AssetName;
	}
	if (ImGui::BeginCombo(name.CStr(), preview.CStr(), 0))
    {
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

#endif
