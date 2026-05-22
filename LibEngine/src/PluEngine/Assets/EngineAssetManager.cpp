//
// Created by Plutex on 5/20/26.
//

#include "PluEngine/Assets/EngineAssetManager.h"

#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluTypes.h"

#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/AssetLoader.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Reflection/TypeTraits.h"


void Plu::EngineAssetManager::DispatchAssetSaveBinary(PluUUID uuid)
{
    PLU_CORE_WARN("Binary Asset Saving is not supported rn. Expect this in the future");
}

void Plu::EngineAssetManager::DispatchAssetSaveJSON(PluUUID uuid)
{
    JSON json = TypeSerializer<TypeInfo*>::Serialize(GetAssetDescriptor(uuid)->AssetType, GetAssetData(uuid).GetRaw());
    json["uuid"] = uuid.getUUID();
    DiskManager::SaveJson(GetAssetDescriptor(uuid)->AssetPath.ToString().ToWide(), json);
}

void Plu::EngineAssetManager::LoadJSONAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    std::optional<nlohmann::json> jsonOpt = DiskManager::LoadJson(assetDesc->AssetPath.ToString().ToWide());
    if (!jsonOpt.has_value()) return;
    const nlohmann::json& json = jsonOpt.value();
    if (!json.contains("typeName")) {
        PLU_CORE_ERROR("Asset at: {} is invalid JSON format", assetDesc->AssetPath.ToString().CStr());
        return;
    }
    String typeName = json["typeName"].get<std::string>().c_str();
    if (mAssetLoaders.Contains(typeName)) {
        mAssetLoaders[typeName]->DispatchAssetLoad(assetDesc, mApplicationInfo->AppAssetManager,
                                                   mApplicationInfo->AppObjectManager,
                                                   mApplicationInfo->AppScenesManager,
                                                   mApplicationInfo->AppShaderManager);
        return;
    }
    TypeInfo* assetType = TypeRegistry::GetInstance()->GetTypeOfName(typeName);
    if (!assetType) return;
    DeserializationContext* dc = new DeserializationContext();
    dc->assetManager = mApplicationInfo->AppAssetManager;
    dc->scenesManager = mApplicationInfo->AppScenesManager;
    dc->shaderManager = mApplicationInfo->AppShaderManager;
    void* loadedAsset = assetType->DeSerializeFromJSON(dc, json);
    delete dc;
    TOwningPointer<IAssetData> loadedAssetInfo = TOwningPointer(static_cast<IAssetData *>(loadedAsset));
    mAssetDataMap.Insert(loadedAssetInfo->Uuid, loadedAssetInfo);
    PLU_CORE_TRACE("Loaded Asset data! UUID {} Type {}", loadedAssetInfo->Uuid.getUUID(), mAssetMap[loadedAssetInfo->Uuid]->AssetType->TypeName.CStr());
    //TODO
    PathW pathToSend = assetDesc->AssetPath.ToString().ToWide();
    GetObjectEventDispatcher()->Dispatch("NewAsset", &pathToSend);
}

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

void Plu::EngineAssetManager::RegisterAssetDataFromLoader(TOwningPointer<IAssetData> assetData,
    TUsePointer<AssetDescriptor> assetDesc)
{
    mAssetDataMap.Insert(assetDesc->Uuid, assetData);
    PLU_CORE_TRACE("Asset Data loaded by loader UUID", assetDesc->Uuid.getUUID());
}

Plu::EngineAssetManager::EngineAssetManager()
{
}

Plu::EngineAssetManager::~EngineAssetManager()
{
}

void Plu::EngineAssetManager::Initialize(ApplicationInfo *appInfo)
{
    mApplicationInfo = appInfo;
    PrepareLoaders();
}

void Plu::EngineAssetManager::PrepareLoaders()
{
    auto typeMap = TypeRegistry::GetInstance()->GetTypeMap();
    for (auto& entry : *typeMap) {
        if (entry.second->IsDerivedOf(IAssetLoader::GetStaticClass()))
        {
            if (mAssetLoaders.Contains(entry.first)) continue;
            TUsePointer<IAssetLoader> loader = mApplicationInfo->AppObjectManager->CreateObject(entry.second);
            mAssetLoaders.Insert(loader->GetSupportedAssetType(),mApplicationInfo->AppObjectManager->GetObjectAsOwner<IAssetLoader>(*loader->GetEngineObjectHandle()));
            PLU_CORE_TRACE("Added Asset Loader of type {}", loader->GetClass()->TypeName.CStr());
        }
    }
}

void Plu::EngineAssetManager::LoadAssetDescriptor(Path assetPath)
{
    if (!assetPath.HasExtension()) return;
    if (assetPath.GetExtension() == PLU_BINARY_EXT) LoadBinaryDescriptor(assetPath);
    if (assetPath.GetExtension() == PLU_ASSET_EXT || assetPath.GetExtension() == PLU_SCENE_EXT) LoadJSONDescriptor(assetPath);
}

void Plu::EngineAssetManager::ScanDirectory(const Path &assetPath)
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(assetPath.CStr())) {
        LoadAssetDescriptor(entry.path().string().c_str());
    }
}

void Plu::EngineAssetManager::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    if (!mApplicationInfo) {
        PLU_CORE_ERROR("Asset Manager is not initialized!");
        return;
    }
    if (assetDesc->LoaderType == AssetLoaderType::JSON) LoadJSONAssetData(assetDesc);
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
    if (!mAssetDataMap.Contains(uuid)) {
        return nullptr;
    }
    return *mAssetDataMap.Find(uuid);
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    return GetAssetData(assetDesc->Uuid);
}

Plu::TUsePointer<Plu::IAssetLoader> Plu::EngineAssetManager::GetAssetLoader(TypeInfo *type)
{
    if (mAssetLoaders.Contains(type->TypeName)) {
        return mAssetLoaders[type->TypeName];
    }
    return nullptr;
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
    if (!mAssetPathMap.Contains(uuid)) {
        return "";
    }
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

bool Plu::EngineAssetManager::AssetExistsWithName(String assetName)
{
    for (auto asset : mAssetMap) {
        if (asset.second->AssetName == assetName) return true;
    }
    return false;
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

void Plu::EngineAssetManager::SaveAsset(TUsePointer<AssetDescriptor> assetDesc)
{
    SaveAsset(assetDesc->Uuid);
}

void Plu::EngineAssetManager::SaveAsset(PluUUID uuid)
{
    if (GetAssetDescriptor(uuid)->LoaderType == AssetLoaderType::Undefined) return;
    if (GetAssetDescriptor(uuid)->LoaderType == AssetLoaderType::Binary)
    {
        DispatchAssetSaveBinary(uuid);
        return;
    }
    DispatchAssetSaveJSON(uuid);
}

void Plu::EngineAssetManager::SaveAsset(TUsePointer<IAssetData> assetDesc)
{
}
