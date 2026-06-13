//
// Created by Plutex on 5/21/26.
//

#include "EditorAssetCreator.h"

#include "PluEngine/Reflection/TypeTraits.h"
#include "EditorAppContext.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

void Plu::EditorAssetCreator::Initialize(TypeInfo *assetClass, const TUsePointer<EngineAssetManager> &assetManager)
{
    mAssetManager = assetManager;
    mTypeInfo = assetClass;
}

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

void Plu::EditorAssetCreator::RenderUI()
{
    static TOwningPointer<IAssetData> newAsset;
    //Show this monstrosity!!
    static GameHashMap<String, DynamicArray<EngineObjectHandle>> objectsPerUuidField;
    static GameHashMap<String, DynamicArray<TUsePointer<AssetDescriptor>>> assetsPerUuidField;
    static GameHashMap<String, int> selectedObjectInUuid;
    if (mFirstTime) {
        ImGui::OpenPopup("Asset Creator");
        void* newObj = mTypeInfo->Construct();
        IAssetData* newAObj =static_cast<IAssetData *>(newObj);
        newAsset = TOwningPointer(newAObj);
        mFirstTime = false;
    }
    if (ImGui::BeginPopupModal("Asset Creator")) {
            static std::string assetName;
            static bool invalidName = false;
            static bool firstTime = true;
            bool startedWithBad = invalidName;
            if (startedWithBad) {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
            if (ImGui::InputText("Asset Name", &assetName) || firstTime) {
                firstTime = false;
                bool found = mAssetManager->AssetExistsWithName(assetName.c_str());
                String pluAssetName = assetName.c_str();
                pluAssetName.Replace(" ", "");
                if (!found) found = pluAssetName.IsEmpty();
                invalidName = found;
            }
            if (startedWithBad) {
                ImGui::SetItemTooltip("Asset name is invalid");
                ImGui::PopStyleColor(3);
            }
            TypeSerializer<TypeInfo*>::EditorControl(mTypeInfo, newAsset.GetRaw());
            std::function<void()> cleanup = [this]() {
                newAsset = nullptr;
                assetName = "";
                selectedObjectInUuid.Clear();
                objectsPerUuidField.Clear();
                invalidName = false;
                firstTime = true;
                GetObjectEventDispatcher()->Dispatch("Finito", nullptr);
            };
            ImGui::BeginDisabled(invalidName);
            if (ImGui::Button("Create")) {
                PathW assetPath = gEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory();
                assetPath += L"/" + StringW::FromNarrow(assetName.c_str()) + PLU_ASSET_EXT_W;
                nlohmann::json assetJson;
                newAsset->Uuid = PluUUID();
                assetJson = mTypeInfo->SerializeToJSON(newAsset.GetRaw());
                assetJson["uuid"] = newAsset->Uuid.getUUID();
                DiskManager::SaveJson(assetPath.ToString(), assetJson);
                mAssetManager->LoadAssetDescriptor(assetPath.ToString().ToNarrow());
                ImGui::CloseCurrentPopup();
                cleanup();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                cleanup();
            }
            ImGui::EndPopup();
        }
}
