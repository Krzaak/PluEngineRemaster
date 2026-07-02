//
// Created by Plutex on 5/25/26.
//

#include "EditorAssetImporter.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "PluEngine/Application.h"
#include "PluEngine/Assets/AssetLoader.h"
#include "PluEngine/Assets/EngineAssetManager.h"

void Plu::EditorAssetImporter::Initialize(DynamicArray<Path> assetPaths, ApplicationInfo *appInfo)
{
    mAssetPaths = assetPaths;
    mApplicationInfo = appInfo;

    String extension = assetPaths[0].GetExtension();
    for (auto path : mAssetPaths) {
        if (path.GetExtension() != extension) {
            GetObjectEventDispatcher()->Dispatch("Finito");
            return;
        }
    }

    mAssetLoader = mApplicationInfo->AppAssetManager->GetAssetLoaderForExtension(extension);
    if (!mAssetLoader) {
        GetObjectEventDispatcher()->Dispatch("Finito");
        return;
    }
    mAssetImportSettingsType = mAssetLoader->GetImportSettingsClass();
}

extern Plu::EditorAppContext* gEditorAppContext;

void Plu::EditorAssetImporter::RenderUI()
{
    if (!mAssetLoader)
    {
        GetObjectEventDispatcher()->Dispatch("Finito");
        return;
    }
    if (!mAssetImportSettingsType) {
        mAssetLoader->HandleAssetImporting(mAssetPaths,
                    gEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory().ToString().ToNarrow(),
                    nullptr,
                    mApplicationInfo->AppAssetManager,
                    mApplicationInfo->AppObjectManager
            );
        GetObjectEventDispatcher()->Dispatch("Finito");
        return;
    }
    if (mFirstTime) {
        ImGui::OpenPopup("Asset Import Settings");
        mAssetImportSettings = mAssetImportSettingsType->Construct();
        mFirstTime = false;
    }
    auto cleanup = [this]() {
        delete mAssetImportSettings;
        mAssetImportSettings = nullptr;
        mAssetPaths.Clear();
        mApplicationInfo = nullptr;
        mAssetLoader = nullptr;
    };
    if (ImGui::BeginPopupModal("Asset Import Settings")) {
        TypeSerializer<TypeInfo*>::EditorControl(mAssetImportSettingsType, mAssetImportSettings);
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            cleanup();
            GetObjectEventDispatcher()->Dispatch("Finito");
        }
        if (ImGui::Button("Import")) {
            mAssetLoader->HandleAssetImporting(mAssetPaths,
                    gEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory().ToString().ToNarrow(),
                    mAssetImportSettings,
                    mApplicationInfo->AppAssetManager,
                    mApplicationInfo->AppObjectManager
            );
            ImGui::CloseCurrentPopup();
            cleanup();
            GetObjectEventDispatcher()->Dispatch("Finito");
        }
        ImGui::EndPopup();
    }
}
