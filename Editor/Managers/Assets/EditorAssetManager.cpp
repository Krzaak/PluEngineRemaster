//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include <filesystem>

#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUUID.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"

bool Plu::EditorAssetManager::LoadAsset(StringW path)
{
    PLU_TRACE("Asset at: {}", String::FromWide(path.CStr()).CStr());
    return true;
}

Plu::EditorAssetManager::EditorAssetManager()
{
    mAssets.Reserve(1000);
}

Plu::EditorAssetManager::~EditorAssetManager()
{
}

Plu::IAssetInfo * Plu::EditorAssetManager::GetAssetByUUID(PluUUID uuid)
{
    return nullptr;
}

bool Plu::EditorAssetManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager)
{
    mEditorProjectManager = editorProjectManager;
    mEngineObjectManager = engineObjectManager;
    for (TypeInfo *importer: mAssetImportersTypes) {
        mAssetImporters.PushBack(DynamicCast<IEditorAssetImporter>(mEngineObjectManager->CreateObject(importer)));
    }
    bool fail = false;
    for (std::filesystem::directory_entry file: std::filesystem::recursive_directory_iterator(mEditorProjectManager->GetProjectAssetsDirectory().CStr())) {
        if (file.is_directory()) continue;
        if (!file.is_regular_file()) continue;
        if (file.path().extension() != PLU_ASSET_EXT_W) continue;
        fail = !LoadAsset(file.path().generic_wstring().c_str());
    }
    return fail;
}

bool Plu::EditorAssetManager::Shutdown()
{
    return true;
}

void Plu::EditorAssetManager::ImportAssets(DynamicArray<PathW> Assets, PathW LoadTo)
{
    for (PathW& asset : Assets)
    {
        for (TUsePointer<IEditorAssetImporter> importer: mAssetImporters)
        {
            if (importer->GetImportableExtensions().Contains(asset.GetExtension().ToNarrow()))
            {
                importer->ImportAsset(asset, LoadTo);
            }
        }
    }
}
