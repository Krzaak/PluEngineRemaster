//
// Created by Plutex on 5/20/26.
//

#include "PluEngine/Assets/EngineAssetManager.h"

#include "PluEngine/Application.h"
#include "PluEngine/Timer.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/PluUtils.h"

#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/AssetLoader.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Reflection/TypeTraits.h"


#ifdef PLU_ENGINE_EDITOR_BUILD
void Plu::EngineAssetManager::DispatchAssetSaveBinary(PluUUID uuid)
{
    CheckOwnerThread();
    TUsePointer<AssetDescriptor> assetDesc = GetAssetDescriptor(uuid);
    if (mAssetLoaders.Contains(assetDesc->AssetType->TypeName)) {
        bool saved = mAssetLoaders[assetDesc->AssetType->TypeName]->DispatchAssetSave(assetDesc,
                mApplicationInfo->AppAssetManager,
                mApplicationInfo->AppObjectManager,
                mApplicationInfo->AppScenesManager,
                mApplicationInfo->AppShaderManager
        );
        if (saved) {
            PLU_CORE_TRACE("Asset Saved by Binary! UUID {}", uuid.getUUID());
            return;
        }
    }
    PLU_CORE_WARN("Binary Asset Saving failed: no loader handled UUID {}", uuid.getUUID());
}

void Plu::EngineAssetManager::DispatchAssetSaveJSON(PluUUID uuid)
{
    CheckOwnerThread();
    if (mAssetLoaders.Contains(GetAssetDescriptor(uuid)->AssetType->TypeName)) {
        TUsePointer<AssetDescriptor> assetDesc = GetAssetDescriptor(uuid);
        bool saved = mAssetLoaders[assetDesc->AssetType->TypeName]->DispatchAssetSave(assetDesc,
                mApplicationInfo->AppAssetManager,
               mApplicationInfo->AppObjectManager,
               mApplicationInfo->AppScenesManager,
               mApplicationInfo->AppShaderManager
        );
        if (saved) {
            PLU_CORE_TRACE("Asset Saved by JSON! UUID {}", uuid.getUUID());
            return;
        }
    }
    JSON json = TypeSerializer<TypeInfo*>::Serialize(GetAssetDescriptor(uuid)->AssetType, GetAssetData(uuid).GetRaw());
    json["uuid"] = uuid.getUUID();
    DiskManager::SaveJson(GetAssetDescriptor(uuid)->AssetPath.ToString().ToWide(), json);
    PLU_CORE_TRACE("Asset Saved by JSON! UUID {}", uuid.getUUID());
}
#endif

void Plu::EngineAssetManager::LoadJSONAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    CheckOwnerThread();
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
    dc = nullptr;
    TOwningPointer<IAssetData> loadedAssetInfo = TOwningPointer(static_cast<IAssetData *>(loadedAsset));
    {
        std::unique_lock lock(mMutex);
        mAssetDataMap.Insert(loadedAssetInfo->Uuid, loadedAssetInfo);
    }
    UInt64 uuidToSend = assetDesc->Uuid;
    GetObjectEventDispatcher()->Dispatch("LoadedAssetData", &uuidToSend);
}

void Plu::EngineAssetManager::LoadBinaryAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    CheckOwnerThread();
    if (mAssetLoaders.Contains(assetDesc->AssetType->TypeName)) {
        mAssetLoaders[assetDesc->AssetType->TypeName]->DispatchAssetLoad(assetDesc, mApplicationInfo->AppAssetManager,
                                                   mApplicationInfo->AppObjectManager,
                                                   mApplicationInfo->AppScenesManager,
                                                   mApplicationInfo->AppShaderManager);
        UInt64 uuidToSend = assetDesc->Uuid;
        GetObjectEventDispatcher()->Dispatch("LoadedAssetData", &uuidToSend);
        return;
    }
    PLU_CORE_ERROR("No Loader for asset UUID {} Type {}", assetDesc->Uuid.getUUID(), assetDesc->AssetType->TypeName.CStr());
}

Plu::PluUUID Plu::EngineAssetManager::LoadJSONDescriptor(const Path &assetPath)
{
    CheckOwnerThread();
    auto json = DiskManager::LoadJson(assetPath.ToString().ToWide());
    if (!json.has_value()) return 0;
    TOwningPointer<AssetDescriptor> assetDescriptor = CreateOwning<AssetDescriptor>();
    String typeName = json.value()["typeName"].get<std::string>().c_str();
    UInt64 uuid = 0;
    if (json.value().contains("uuid")) {
        uuid = json.value()["uuid"].get<UInt64>();
    }
    if (uuid == 0) {
        PLU_CORE_CRITICAL("No uuid field in asset at {}!", assetPath.ToString().CStr());
        return 0;
    }
    assetDescriptor->AssetType = TypeRegistry::GetInstance()->GetTypeOfName(typeName);
    assetDescriptor->LoaderType = AssetLoaderType::JSON;
    assetDescriptor->Uuid = uuid;
    assetDescriptor->AssetPath = assetPath;
#ifdef PLU_ENGINE_EDITOR_BUILD
    String assetName = "";
    if (json.value().contains("assetName")) {
        assetName = json.value()["assetName"].get<std::string>().c_str();
    } else {
        assetName = assetPath.GetStem();
    }
    assetDescriptor->AssetName = assetName;
#endif
    {
        std::unique_lock lock(mMutex);
        mAssetPathByUUIDMap.Insert(assetPath, uuid);
        mAssetMap.Insert(uuid, assetDescriptor);
        mAssetPathMap.Insert(uuid, assetPath);
    }
#ifdef PLU_ENGINE_EDITOR_BUILD
    PLU_CORE_TRACE("New JSON asset descriptor loaded! UUID {} Path {}", uuid, assetPath.ToString().CStr());
#else
    PLU_CORE_TRACE("New JSON asset descriptor loaded! UUID {}", uuid);
#endif
    return uuid;
}

Plu::PluUUID Plu::EngineAssetManager::LoadBinaryDescriptor(Path assetPath)
{
    CheckOwnerThread();
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

    // Nagłówek (magic/typeName/uuid) jest niezależny od wersji formatu danych,
    // więc akceptujemy każdą znaną wersję (tekstury=1, mesh=2). Walidacja samych
    // danych odbywa się w loaderze konkretnego typu (np. LoadStaticMesh).
    if (magic != 0x41554C50 || (version != 1 && version != 2))
    {
        PLU_ERROR("File {} has invalid magic or version!", assetPath.CStr());
        fclose(file);
        return 0;
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
    assetDescriptor->AssetPath = assetPath;
#ifdef PLU_ENGINE_EDITOR_BUILD
    String assetName = assetPath.GetStem();
    assetDescriptor->AssetName = assetName;
#endif
    {
        std::unique_lock lock(mMutex);
        mAssetPathByUUIDMap.Insert(assetPath, uuid);
        mAssetMap.Insert(uuid, assetDescriptor);
        mAssetPathMap.Insert(uuid, assetPath);
    }
#ifdef PLU_ENGINE_EDITOR_BUILD
    PLU_CORE_TRACE("New BINARY asset descriptor loaded! UUID {} Path {}", uuid, assetPath.ToString().CStr());
#else
    PLU_CORE_TRACE("New BINARY asset descriptor loaded! UUID {}", uuid);
#endif
    return uuid;
}

void Plu::EngineAssetManager::RegisterAssetDataFromLoader(TOwningPointer<IAssetData> assetData,
    TUsePointer<AssetDescriptor> assetDesc)
{
    CheckOwnerThread();
    {
        std::unique_lock lock(mMutex);
        mAssetDataMap.Insert(assetDesc->Uuid, assetData);
    }
    PLU_CORE_TRACE("Asset Data loaded by loader UUID {}", assetDesc->Uuid.getUUID());
}

static Plu::TUsePointer<Plu::EngineAssetManager> gEngineAssetManager;

Plu::EngineAssetManager::EngineAssetManager()
{
}

Plu::EngineAssetManager::~EngineAssetManager()
{
}

void Plu::EngineAssetManager::Initialize(ApplicationInfo *appInfo)
{
    CheckOwnerThread();
    mApplicationInfo = appInfo;
    gEngineAssetManager = mApplicationInfo->AppObjectManager->GetObjectAsUser<EngineAssetManager>(*GetEngineObjectHandle());
    PrepareLoaders();
}

void Plu::EngineAssetManager::PrepareLoaders()
{
    CheckOwnerThread();
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
    CheckOwnerThread();
    PluUUID uuid;
#ifdef PLU_ENGINE_EDITOR_BUILD
    if (!assetPath.HasExtension()) return;
    if (assetPath.GetExtension() == PLU_BINARY_EXT) uuid = LoadBinaryDescriptor(assetPath);
    if (assetPath.GetExtension() == PLU_ASSET_EXT || assetPath.GetExtension() == PLU_SCENE_EXT) uuid = LoadJSONDescriptor(assetPath);
#else
    if (assetPath.HasExtension()) return;
    String stem = assetPath.GetStem();
    String assetTxt = "Asset";
    String UUID = stem.Substring(assetTxt.Length() + 1);
    if (stem[0] == 'j') uuid = LoadJSONDescriptor(assetPath);
    if (stem[0] == 'b') uuid = LoadBinaryDescriptor(assetPath);
#endif

    UInt64 uuidToSend = uuid;
    GetObjectEventDispatcher()->Dispatch("LoadAssetDescriptor", &uuidToSend);
}

void Plu::EngineAssetManager::ScanDirectory(const Path &assetPath, bool engineAssets)
{
    CheckOwnerThread();
    PLU_CORE_TRACE("Scan directory: {}", assetPath.ToString().CStr());
    for (auto& entry : std::filesystem::recursive_directory_iterator(assetPath.CStr())) {
        if (entry.is_directory()) continue;
        Path entryPath = entry.path().string().c_str();
        LoadAssetDescriptor(entryPath);
#ifdef PLU_ENGINE_EDITOR_BUILD
        if (TUsePointer<AssetDescriptor> assetDesc = GetAssetDescriptor(entryPath)) {
            assetDesc->IsEngineAsset = engineAssets;
        }
#endif
    }
}

void Plu::EngineAssetManager::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    CheckOwnerThread();
    if (!mApplicationInfo) {
        PLU_CORE_ERROR("Asset Manager is not initialized!");
        return;
    }
    if (!assetDesc) {
        PLU_CORE_ERROR("Asset Descriptor is invalid!");
        return;
    }
    PLU_PROFILE_SCOPE("LoadAssetData");
    if (assetDesc->LoaderType == AssetLoaderType::JSON) LoadJSONAssetData(assetDesc);
    if (assetDesc->LoaderType == AssetLoaderType::Binary) LoadBinaryAssetData(assetDesc);
}

Plu::TUsePointer<Plu::AssetDescriptor> Plu::EngineAssetManager::GetAssetDescriptor(PluUUID uuid)
{
    std::shared_lock lock(mMutex);
    const auto data = mAssetMap.Find(uuid);
    if (!data) return nullptr;
    return *data;
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(PluUUID uuid)
{
    PLU_PROFILE_SCOPE("GetAssetData");
    // Fast path: already loaded — safe from any thread
    {
        std::shared_lock lock(mMutex);
        auto found = mAssetDataMap.Find(uuid);
        if (found) return *found;
    }
    // Lazy-load: I/O + possible GL setup — main thread only
    PLU_CORE_ASSERT(IsOnMainThread(), "GetAssetData: asset not loaded and called off main thread; use GetAssetDataNoLoad");
    TUsePointer<AssetDescriptor> desc;
    {
        std::shared_lock lock(mMutex);
        auto found = mAssetMap.Find(uuid);
        if (found) desc = *found;
    }
    if (desc) LoadAssetData(desc); // internally takes unique_lock for Insert
    std::shared_lock lock(mMutex);
    auto found = mAssetDataMap.Find(uuid);
    if (!found) return nullptr;
    return *found;
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(TUsePointer<AssetDescriptor> assetDesc)
{
    return GetAssetData(assetDesc->Uuid);
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetDataNoLoad(PluUUID uuid) const
{
    std::shared_lock lock(mMutex);
    auto found = mAssetDataMap.Find(uuid);
    // NB: a `found ? *found : nullptr` ternary would force a common type of
    // TOwningPointer<IAssetData>, copy-constructing a temporary owner — which trips
    // PLU_PTR_ASSERT_OWNER off the owning (main) thread. Construct the TUsePointer directly.
    if (!found) return nullptr;
    return *found;
}

void Plu::EngineAssetManager::RequestAssetDataLoad(PluUUID uuid)
{
    // Callable from any thread (typically the render thread on a GetAssetDataNoLoad miss).
    // Cheap fast-out if already loaded so we don't queue work the main thread would no-op.
    if (IsAssetLoaded(uuid)) return;
    std::lock_guard lock(mPendingLoadMutex);
    mPendingLoadRequests.Insert(uuid.getUUID());
}

void Plu::EngineAssetManager::ProcessPendingLoads()
{
    PLU_PROFILE_SCOPE("EngineAssetManager::ProcessPendingLoads");
    PLU_CORE_ASSERT(IsOnMainThread(), "ProcessPendingLoads must run on the main thread");

    // Swap the queue out under the lock so off-main threads can keep posting while we do I/O.
    HashSet<UInt64> pending;
    {
        std::lock_guard lock(mPendingLoadMutex);
        if (mPendingLoadRequests.IsEmpty()) return;
        pending = std::move(mPendingLoadRequests);
        mPendingLoadRequests.Clear();
    }
    // GetAssetData performs the I/O on the main thread and populates the cache; the render
    // thread will see the data via GetAssetDataNoLoad on a subsequent frame.
    for (UInt64 uuid : pending) {
        GetAssetData(uuid);
    }
}

Plu::TUsePointer<Plu::IAssetLoader> Plu::EngineAssetManager::GetAssetLoader(TypeInfo *type)
{
    std::shared_lock lock(mMutex);
    if (mAssetLoaders.Contains(type->TypeName)) {
        return mAssetLoaders[type->TypeName];
    }
    return nullptr;
}

bool Plu::EngineAssetManager::AssetExists(PluUUID uuid) const
{
    std::shared_lock lock(mMutex);
    return mAssetMap.Contains(uuid);
}

#ifdef PLU_ENGINE_EDITOR_BUILD
Plu::TUsePointer<Plu::AssetDescriptor> Plu::EngineAssetManager::GetAssetDescriptor(Path assetPath)
{
    std::shared_lock lock(mMutex);
    if (!mAssetPathByUUIDMap.Contains(assetPath)) return nullptr;
    return *mAssetMap.Find(*mAssetPathByUUIDMap.Find(assetPath));
}

Plu::TUsePointer<Plu::IAssetData> Plu::EngineAssetManager::GetAssetData(Path assetPath)
{
    CheckOwnerThread();
    UInt64 uuid;
    TUsePointer<AssetDescriptor> desc;
    {
        std::shared_lock lock(mMutex);
        if (!mAssetPathByUUIDMap.Contains(assetPath)) return nullptr;
        uuid = *mAssetPathByUUIDMap.Find(assetPath);
        auto dataFound = mAssetDataMap.Find(uuid);
        if (dataFound) return *dataFound;
        auto descFound = mAssetMap.Find(uuid);
        if (descFound) desc = *descFound;
    }
    if (desc) LoadAssetData(desc); // internally takes unique_lock for Insert
    std::shared_lock lock(mMutex);
    auto found = mAssetDataMap.Find(uuid);
    if (!found) return nullptr;
    return *found;
}

Plu::TUsePointer<Plu::IAssetLoader> Plu::EngineAssetManager::GetAssetLoaderForExtension(String extension)
{
    std::shared_lock lock(mMutex);
    for (const auto& loader : mAssetLoaders) {
        if (loader.second->GetSupportedImportExtensions().Contains(extension)) return loader.second;
    }
    return nullptr;
}

#endif

Plu::Path Plu::EngineAssetManager::GetAssetPath(PluUUID uuid)
{
    std::shared_lock lock(mMutex);
    if (!mAssetPathMap.Contains(uuid)) {
        return "";
    }
    return *mAssetPathMap.Find(uuid);
}

Plu::Path Plu::EngineAssetManager::GetAssetPath(TUsePointer<AssetDescriptor> assetDesc)
{
    std::shared_lock lock(mMutex);
    return *mAssetPathMap.Find(assetDesc->Uuid);
}

bool Plu::EngineAssetManager::AssetExistsInPath(Path assetPath) const
{
    std::shared_lock lock(mMutex);
    return mAssetPathByUUIDMap.Contains(assetPath);
}

bool Plu::EngineAssetManager::IsAssetLoaded(PluUUID uuid) const
{
    std::shared_lock lock(mMutex);
    return mAssetDataMap.Contains(uuid);
}

#ifdef PLU_ENGINE_EDITOR_BUILD

bool Plu::EngineAssetManager::AssetExistsWithName(String assetName)
{
    std::shared_lock lock(mMutex);
    for (auto asset : mAssetMap) {
        if (asset.second->AssetName == assetName) return true;
    }
    return false;
}

DynamicArray<Plu::TUsePointer<Plu::AssetDescriptor>> Plu::EngineAssetManager::GetAllAssetDescriptorsOfType(TypeInfo *type)
{
    std::shared_lock lock(mMutex);
    DynamicArray<TUsePointer<AssetDescriptor>> assetDescriptors;
    for (auto& entry : mAssetMap) {
        if (entry.second->AssetType->IsDerivedOfOrSame(type)) assetDescriptors.PushBack(entry.second);
    }
    return assetDescriptors;
}

void Plu::EngineAssetManager::PrepareAssetsForDistribution(Path dir)
{
    CheckOwnerThread();
    Path assetsDir = dir.ToString() + "/ProjectDist";
    std::filesystem::create_directory(assetsDir.CStr());
    for (const auto& asset : mAssetMap) {
        if (asset.second->LoaderType == AssetLoaderType::Undefined) {
            PLU_CORE_ERROR("Cannot prepare asset with Undefined loader type! UUID {}", asset.first);
            continue;
        }
        if (asset.second->LoaderType == AssetLoaderType::Binary) {
            String fileName = asset.second->AssetPath.GetFilename().CStr();
            std::filesystem::copy(asset.second->AssetPath.CStr(), assetsDir.CStr());
            std::filesystem::rename((assetsDir.ToString() + "/" + fileName).CStr(), (assetsDir.ToString() + "/bAsset" + asset.second->Uuid.toString()).CStr());
        } else if (asset.second->LoaderType == AssetLoaderType::JSON) {
            String fileName = asset.second->AssetPath.GetFilename().CStr();
            std::filesystem::copy(asset.second->AssetPath.CStr(), assetsDir.CStr());
            std::filesystem::rename((assetsDir.ToString() + "/" + fileName).CStr(), (assetsDir.ToString() + "/jAsset" + asset.second->Uuid.toString()).CStr());
        }
    }
}

void Plu::EngineAssetManager::ConstructPythonAssetDictionary(Path file)
{
    CheckOwnerThread();
    if (!file.HasFilename()) return;
    if (!file.HasExtension()) return;

    if (file.GetExtension() != ".py") return;

    std::ofstream out(file.CStr(), std::ios::binary);

    auto writeLine = [&out](const String& line) {
        out.write(line.CStr(), static_cast<std::streamsize>(line.Length()));
    };

    // Turn an arbitrary preset/channel/asset name into a valid Python identifier so it can be
    // emitted as a class attribute (e.g. "No Collision!" -> "No_Collision_").
    auto toIdentifier = [](const String& name) -> String {
        String id;
        for (UInt32 i = 0; i < name.Length(); ++i) {
            const char c = name.CStr()[i];
            const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                            (c >= '0' && c <= '9') || c == '_';
            id += ok ? c : '_';
        }
        if (id.IsEmpty() || (id.CStr()[0] >= '0' && id.CStr()[0] <= '9'))
            id = "_" + id;
        return id;
    };

    writeLine("class Assets:\n");
    for (const auto& asset : mAssetMap) {
        writeLine("    " + toIdentifier(asset.second->AssetName) + " = " + asset.second->Uuid.toString() + "\n");
    }

    // Collision config (channels + presets) so scripts can reference the project's UE-style
    // collision data by name instead of hard-coding strings. Profile names feed
    // PhysicsBodyComponent.SetCollisionProfile().
    const CollisionConfig& collision = ActiveCollisionConfig();

    writeLine("\nclass CollisionChannels:\n");
    if (collision.ChannelNames.IsEmpty())
        writeLine("    pass\n");
    for (UInt32 i = 0; i < collision.ChannelNames.Size(); ++i)
        writeLine("    " + toIdentifier(collision.ChannelNames[i]) + " = " + String::FromInt(i) + "\n");

    writeLine("\nclass CollisionProfiles:\n");
    if (collision.Profiles.IsEmpty())
        writeLine("    pass\n");
    for (UInt32 i = 0; i < collision.Profiles.Size(); ++i) {
        const String& name = collision.Profiles[i].Name;
        writeLine("    " + toIdentifier(name) + " = \"" + name + "\"\n");
    }

    out.close();
}

void Plu::EngineAssetManager::MarkAssetDirty(TUsePointer<AssetDescriptor> assetDesc)
{
    mDirtyAssets.Insert(assetDesc->Uuid);
}

bool Plu::EngineAssetManager::IsAssetDirty(TUsePointer<AssetDescriptor> assetDesc) const
{
    return mDirtyAssets.Contains(assetDesc->Uuid);
}

bool Plu::EngineAssetManager::AreAnyAssetsDirty() const
{
    return !mDirtyAssets.IsEmpty();
}

void Plu::EngineAssetManager::SaveAsset(TUsePointer<AssetDescriptor> assetDesc)
{
    CheckOwnerThread();
    SaveAsset(assetDesc->Uuid);
}

void Plu::EngineAssetManager::SaveAsset(PluUUID uuid)
{
    CheckOwnerThread();
    if (GetAssetDescriptor(uuid)->LoaderType == AssetLoaderType::Undefined) return;
    if (GetAssetDescriptor(uuid)->IsEngineAsset) {
        PLU_CORE_WARN("Trying to save Engine Asset! Skipping save");
        return;
    }
    mDirtyAssets.Remove(uuid);
    if (GetAssetDescriptor(uuid)->LoaderType == AssetLoaderType::Binary)
    {
        DispatchAssetSaveBinary(uuid);
        return;
    }
    DispatchAssetSaveJSON(uuid);
}

void Plu::EngineAssetManager::SaveAsset(TUsePointer<IAssetData> assetDesc)
{
    CheckOwnerThread();
    SaveAsset(assetDesc->Uuid);
}
#endif

Plu::TUsePointer<Plu::IAssetData> Plu::GetAssetByUUID(UInt64 uuid)
{
    return gEngineAssetManager->GetAssetData(uuid);
}

Plu::TUsePointer<Plu::IAssetData> Plu::GetAssetUserAsRaw(IAssetData *assetInfo)
{
    return gEngineAssetManager->GetAssetData(assetInfo->Uuid);
}

