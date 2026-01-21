//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include <filesystem>
#include <utility>

#include "json_fwd.hpp"
#include "detail/meta/std_fs.hpp"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUUID.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

bool Plu::EditorAssetManager::LoadAsset(StringW path)
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
    UInt32 magic = 0;
    UInt32 version = 0;
    fread(&magic, sizeof(UInt32), 1, file);
    fread(&version, sizeof(UInt32), 1, file);

    if (magic != 0x41554C50 || version != 1)
    {
        PLU_ERROR("File {} has invalid ID!", path.ToNarrow().CStr());
        fclose(file);
        return false;
    }

    // Typ assetu
    UInt32 typeLength = 0;
    fread(&typeLength, sizeof(UInt32), 1, file);
    char* typeBuffer = new char[typeLength + 1];
    fread(typeBuffer, sizeof(char), typeLength, file);
    typeBuffer[typeLength] = '\0';
    // Opcjonalnie możesz sprawdzić typ: if (strcmp(typeBuffer, "StaticMesh") != 0) { ... }
    String type = typeBuffer;
    delete[] typeBuffer;
    PLU_INFO("{}", type.CStr());
    for (const TOwningPointer<IEditorAssetHandler>& handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == type) {
            auto asset = handler->LoadAsset(path, mEditorProjectManager, mEngineObjectManager, this);
            PLU_ASSERT(asset, "Asset cannot be null after load!")
            asset->mAssetType = type;
            return true;
        }
    }
    fclose(file);
#ifdef PLU_PLATFORM_WINDOWS
#error "Close dodaj no tam"
#endif

    return false;
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
    for (const TOwningPointer<IEditorAssetHandler>& handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == json["type"].get<std::string>().c_str()) {
            auto asset = handler->LoadAsset(path, mEditorProjectManager, mEngineObjectManager, this);
            PLU_ASSERT(asset, "Asset cannot be null after JSON import!")
            asset->mAssetType = json["type"].get<std::string>().c_str();
            return true;
        }
    }
    return false;
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

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::EditorAssetManager::GetAssetByPath(const PathW& path)
{
    for (std::pair asset: mAssets) {
        if (asset.second->GetAssetPath() == path) return asset.second;
    }
    return nullptr;
}

Plu::TypeInfo * Plu::EditorAssetManager::GetAssetViewportClass(TUsePointer<IEditorAssetObject> assetObject)
{
    String type = assetObject->mAssetType;
    for (auto handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == type) {
            auto viewportClass = handler->GetAssetViewportClass();
            String msg = "No viewport class for ";
            msg += type;
            PLU_ASSERT(viewportClass, msg.CStr())
            return viewportClass;
        }
    }
    return nullptr;
}

void Plu::EditorAssetManager::AddAssetFromHandler(const TOwningPointer<IEditorAssetObject>& assetObject, const PluUUID& uuid, const PathW &path)
{
    assetObject->mAssetPath = path;
    mAssets.Insert(uuid.getUUID(), assetObject);
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
