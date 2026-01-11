//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include <filesystem>

#include "json_fwd.hpp"
#include "detail/meta/std_fs.hpp"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUUID.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

bool Plu::EditorAssetManager::LoadAsset(const StringW& path)
{
    PLU_TRACE("Asset at: {}", String::FromWide(path.CStr()).CStr());
    if (PathW(path).GetExtension() != PLU_BINARY_EXT_W) return LoadAssetJSON(path);
    FILE* file = nullptr;

#ifdef _WIN32
    _wfopen_s(&file, path.CStr(), L"rb");
#else
    file = fopen(path.ToNarrow().CStr(), "rb");
#endif

    if (!file) return false;

    // Sprawdź magic number i wersję
    MaxUInt32 magic = 0;
    MaxUInt32 version = 0;
    fread(&magic, sizeof(MaxUInt32), 1, file);
    fread(&version, sizeof(MaxUInt32), 1, file);

    if (magic != 0x41554C50 || version != 1)
    {
        PLU_ERROR("File {} has invalid ID!", path.ToNarrow().CStr());
        fclose(file);
        return false;
    }

    // Typ assetu
    MaxUInt32 typeLength = 0;
    fread(&typeLength, sizeof(MaxUInt32), 1, file);
    char* typeBuffer = new char[typeLength + 1];
    fread(typeBuffer, sizeof(char), typeLength, file);
    typeBuffer[typeLength] = '\0';
    // Opcjonalnie możesz sprawdzić typ: if (strcmp(typeBuffer, "StaticMesh") != 0) { ... }
    String type = typeBuffer;
    delete[] typeBuffer;
    PLU_INFO("{}", type.CStr());
    for (const TOwningPointer<IEditorAssetHandler>& handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == type) {
            handler->LoadAsset(path, mEditorProjectManager, mEngineObjectManager, this);
        }
    }
    return true;
}

bool Plu::EditorAssetManager::LoadAssetJSON(const PathW& path)
{
    std::optional<nlohmann::json> jsonOpt = DiskManager::LoadJson(path);
    if (!jsonOpt.has_value()) return false;
    const nlohmann::json& json = jsonOpt.value();
    if (!json.contains("type")) {
        PLU_ERROR("Asset at: {} is invalid JSON format");
        return false;
    }
    PLU_TRACE("Loading asset of type: {}", json["type"].get<std::string>().c_str());
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

void Plu::EditorAssetManager::AddAssetFromHandler(TOwningPointer<IEditorAssetObject> assetObject, const PluUUID& uuid)
{
    mAssets.Insert(uuid.getUUID(), std::move(assetObject));
}

bool Plu::EditorAssetManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager)
{
    mEditorProjectManager = editorProjectManager;
    mEngineObjectManager = engineObjectManager;
    for (TypeInfo *importer: mAssetImportersTypes) {
        mAssetImporters.PushBack(DynamicCast<IEditorAssetHandler>(mEngineObjectManager->CreateObject(importer)));
    }
    bool fail = false;
    for (std::filesystem::directory_entry file: std::filesystem::recursive_directory_iterator(mEditorProjectManager->GetProjectAssetsDirectory().CStr())) {
        if (file.is_directory()) continue;
        if (!file.is_regular_file()) continue;
        bool asset = file.path().extension() == PLU_ASSET_EXT_W;
        bool scn = file.path().extension() == PLU_SCENE_EXT_W;
        bool bin = file.path().extension() == PLU_BINARY_EXT_W;
        if (scn || asset || bin) {
            fail = !LoadAsset(file.path().generic_wstring().c_str());
        }
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
        for (TUsePointer<IEditorAssetHandler> importer: mAssetImporters)
        {
            if (importer->GetImportableExtensions().Contains(asset.GetExtension().ToNarrow()))
            {
                importer->ImportAsset(asset, LoadTo);
            }
        }
    }
}
