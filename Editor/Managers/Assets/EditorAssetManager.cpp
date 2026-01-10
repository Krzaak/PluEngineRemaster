//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include <filesystem>

#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUUID.h"

bool Plu::EditorAssetManager::LoadAsset(StringW path)
{
    PLU_TRACE("Asset at: {}", String::FromWide(path.CStr()).CStr());
    return true;
}

Plu::EditorAssetManager::EditorAssetManager()
{
}

Plu::EditorAssetManager::~EditorAssetManager()
{
}

Plu::IAssetInfo * Plu::EditorAssetManager::GetAssetByUUID(PluUUID uuid)
{
    return nullptr;
}

bool Plu::EditorAssetManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager)
{
    mEditorProjectManager = editorProjectManager;
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
