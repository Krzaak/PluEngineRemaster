//
// Created by Plutex on 5/20/26.
//

#include "PluEngine/Assets/EngineAssetManager.h"

#include "PluEngine/PluPaths.h"

#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Managers/DiskManager.h"


void Plu::EngineAssetManager::LoadJSONDescriptor(const Path &assetPath)
{
    auto json = DiskManager::LoadJson(assetPath.ToString().ToWide());
    if (!json.has_value()) return;
    TOwningPointer<AssetDescriptor> assetDescriptor = CreateOwning<AssetDescriptor>();
    String typeName = json.value()["typeName"].get<std::string>().c_str();
    UInt64 uuid = 0;
    if (json.value().contains("uuid")) {
        uuid = json.value()["uuid"].get<UInt64>();
    }
    if (uuid == 0) {
        PLU_CORE_CRITICAL("No uuid field in asset at {}!", assetPath.ToString().CStr());
        return;
    }
    assetDescriptor->AssetType = TypeRegistry::GetInstance()->GetTypeOfName(typeName);
    assetDescriptor->LoaderType = AssetLoaderType::JSON;
    assetDescriptor->Uuid = uuid;
#ifdef PLU_ENGINE_EDITOR_BUILD
    String assetName = "";
    if (json.value().contains("assetName")) {
        assetName = json.value()["assetName"].get<std::string>().c_str();
    } else {
        assetName = assetPath.GetStem();
    }
    assetDescriptor->AssetName = assetName;
    assetDescriptor->AssetPath = assetPath;
    mAssetPathByUUIDMap.Insert(assetPath, uuid);
#endif
    mAssetMap.Insert(uuid, assetDescriptor);
    mAssetPathMap.Insert(uuid, assetPath);
#ifdef PLU_ENGINE_EDITOR_BUILD
    PLU_CORE_TRACE("New JSON asset descriptor loaded! UUID {} Path {}", uuid, assetPath.ToString().CStr());
#else
    PLU_CORE_TRACE("New JSON asset descriptor loaded! UUID {}", uuid);
#endif
}

void Plu::EngineAssetManager::LoadBinaryDescriptor(Path assetPath)
{
    FILE* file = nullptr;

#ifdef _WIN32
    _wfopen_s(&file, StringW::FromNarrow(assetPath.CStr()).CStr(), L"rb");
#else
    file = fopen(assetPath.CStr(), "rb");
#endif
    UInt32 magic = 0;
    UInt32 version = 0;
    fread(&magic, sizeof(UInt32), 1, file);
    fread(&version, sizeof(UInt32), 1, file);

    if (magic != 0x41554C50 || version != 1)
    {
        PLU_ERROR("File {} has invalid magic or version!", assetPath.CStr());
        fclose(file);
        return;
    }

    UInt32 typeLength = 0;
    fread(&typeLength, sizeof(UInt32), 1, file);
    char* typeBuffer = new char[typeLength + 1];
    fread(typeBuffer, sizeof(char), typeLength, file);
    typeBuffer[typeLength] = '\0';
    String typeName = typeBuffer;
    delete[] typeBuffer;

    UInt64 uuid;
    fread(&uuid, sizeof(UInt64), 1, file);

    TOwningPointer<AssetDescriptor> assetDescriptor = CreateOwning<AssetDescriptor>();
    assetDescriptor->Uuid = uuid;
    assetDescriptor->AssetType = TypeRegistry::GetInstance()->GetTypeOfName(typeName);
    assetDescriptor->LoaderType = AssetLoaderType::Binary;
#ifdef PLU_ENGINE_EDITOR_BUILD
    String assetName = assetPath.GetStem();
    assetDescriptor->AssetName = assetName;
    assetDescriptor->AssetPath = assetPath;
    mAssetPathByUUIDMap.Insert(assetPath, uuid);
#endif
    mAssetMap.Insert(uuid, assetDescriptor);
    mAssetPathMap.Insert(uuid, assetPath);
#ifdef PLU_ENGINE_EDITOR_BUILD
    PLU_CORE_TRACE("New BINARY asset descriptor loaded! UUID {} Path {}", uuid, assetPath.ToString().CStr());
#else
    PLU_CORE_TRACE("New BINARY asset descriptor loaded! UUID {}", uuid);
#endif
}

Plu::EngineAssetManager::EngineAssetManager()
{
}

Plu::EngineAssetManager::~EngineAssetManager()
{
}

void Plu::EngineAssetManager::LoadAssetDescriptor(Path assetPath)
{
    if (!assetPath.HasExtension()) return;
    if (assetPath.GetExtension() == PLU_BINARY_EXT) LoadBinaryDescriptor(assetPath);
    if (assetPath.GetExtension() == PLU_ASSET_EXT) LoadJSONDescriptor(assetPath);
}

void Plu::EngineAssetManager::ScanDirectory(const Path &assetPath)
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(assetPath.CStr())) {
        LoadAssetDescriptor(entry.path().string().c_str());
    }
}

void Plu::EngineAssetManager::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    PLU_CORE_TRACE("Load Asset Data!");
}

Plu::TUsePointer<Plu::AssetDescriptor> Plu::EngineAssetManager::GetAssetDescriptor(PluUUID uuid)
{
    const auto data = mAssetMap.Find(uuid);
    if (!data) return nullptr;
    return *data;
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(PluUUID uuid)
{
    if (!mAssetDataMap.Contains(uuid)) {
        LoadAssetData(mAssetMap[uuid]);
    }
    return *mAssetDataMap.Find(uuid);
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    return GetAssetData(assetDesc->Uuid);
}

bool Plu::EngineAssetManager::AssetExists(PluUUID uuid) const
{
    return mAssetMap.Contains(uuid);
}

Plu::TUsePointer<Plu::AssetDescriptor> Plu::EngineAssetManager::GetAssetDescriptor(Path assetPath)
{
    if (!mAssetPathByUUIDMap.Contains(assetPath)) return nullptr;
    return *mAssetMap.Find(*mAssetPathByUUIDMap.Find(assetPath));
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(Path assetPath)
{
    if (!mAssetPathByUUIDMap.Contains(assetPath)) return nullptr;
    UInt64 uuid = mAssetPathByUUIDMap[assetPath];
    if (!mAssetDataMap.Contains(uuid)) {
        LoadAssetData(mAssetMap[mAssetPathByUUIDMap[assetPath]]);
    }
    return *mAssetDataMap.Find(uuid);
}

Plu::Path Plu::EngineAssetManager::GetAssetPath(PluUUID uuid)
{
    return *mAssetPathMap.Find(uuid);
}

Plu::Path Plu::EngineAssetManager::GetAssetPath(TUsePointer<AssetDescriptor> assetDesc)
{
    return *mAssetPathMap.Find(assetDesc->Uuid);
}

bool Plu::EngineAssetManager::AssetExistsInPath(Path assetPath) const
{
    return mAssetPathByUUIDMap.Contains(assetPath);
}

DynamicArray<Plu::TUsePointer<Plu::AssetDescriptor>> Plu::EngineAssetManager::GetAllAssetDescriptorsOfType(TypeInfo *type)
{
    DynamicArray<TUsePointer<AssetDescriptor>> assetDescriptors;
    for (auto& entry : mAssetMap) {
        if (entry.second->AssetType->IsDerivedOfOrSame(type)) assetDescriptors.PushBack(entry.second);
    }
    return assetDescriptors;
}

void Plu::EngineAssetManager::ImportAssets(DynamicArray<Path> assetPaths, Path importTo)
{
}
