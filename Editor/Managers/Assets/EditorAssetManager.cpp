//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include <filesystem>
#include <utility>

#include "EditorAppContext.h"
#include "json_fwd.hpp"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUUID.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "Managers/Shaders/EditorShaderManager.h"

Plu::EditorTypeRegistry * Plu::EditorTypeRegistry::GetInstance()
{
    static EditorTypeRegistry* editorTypeRegistry;
    if (!editorTypeRegistry) {
        editorTypeRegistry = new EditorTypeRegistry();
    }
    return editorTypeRegistry;
}

void Plu::EditorTypeRegistry::AddConstructor(String name, EditorTypeRegistry::EditorAssetConstructor cons)
{
    mEditorAssetsCreators[name] = std::move(cons);
}

Plu::TOwningPointer<Plu::IEditorAssetObject> Plu::EditorTypeRegistry::ConstructAssetObject(TypeInfo *type, const TOwningPointer<IAssetInfo> &assetInfo)
{
    if (!type) return nullptr;
    return mEditorAssetsCreators.Find(type->TypeName)->operator()(assetInfo);
}

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
            fclose(file);
            if (!asset) {
                return false;
            }
            return true;
        }
    }
    fclose(file);

    return false;
}

bool Plu::EditorAssetManager::LoadAssetJSON(const PathW& path)
{
    std::optional<nlohmann::json> jsonOpt = DiskManager::LoadJson(path);
    if (!jsonOpt.has_value()) return false;
    const nlohmann::json& json = jsonOpt.value();
    if (!json.contains("typeName")) {
        PLU_ERROR("Asset at: {} is invalid JSON format", path.ToString().ToNarrow().CStr());
        return false;
    }
    PLU_TRACE("Loading asset of type: {}", json["typeName"].get<std::string>().c_str());
    for (const TOwningPointer<IEditorAssetHandler>& handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == json["typeName"].get<std::string>().c_str()) {
            auto asset = handler->LoadAsset(path, mEditorProjectManager, mEngineObjectManager, this);
            PLU_ASSERT(asset, "Asset cannot be null after JSON import!")
            return true;
        }
    }
    PLU_WARN("Using default asset loader for type {}", json["typeName"].get<std::string>().c_str());
    TypeInfo* assetType = TypeRegistry::GetInstance()->GetTypeOfName(json["typeName"].get<std::string>().c_str());
    if (!assetType) return false;
    DeserializationContext* dc = new DeserializationContext();
    dc->assetManager = mEditorProjectManager->GetAppContext()->EditorAssetManager;
    dc->scenesManager = mEditorProjectManager->GetAppContext()->EditorScenesManager;
    dc->shaderManager = mEditorProjectManager->GetAppContext()->EditorShaderManager;
    void* loadedAsset = assetType->DeSerializeFromJSON(dc, json);
    delete dc;
    TOwningPointer<IAssetInfo> loadedAssetInfo = TOwningPointer(static_cast<IAssetInfo *>(loadedAsset));
    TOwningPointer<IEditorAssetObject> assetObjectT = EditorTypeRegistry::GetInstance()->ConstructAssetObject(assetType, loadedAssetInfo);
    AddAssetFromHandler(assetObjectT, loadedAssetInfo->Uuid, path, assetType);
    return true;
}

Plu::EditorAssetManager::EditorAssetManager()
{
    mAssets.Reserve(1000);
}

Plu::EditorAssetManager::~EditorAssetManager()
{
}

Plu::TUsePointer<Plu::IAssetInfo> Plu::EditorAssetManager::GetAssetByUUID(PluUUID uuid)
{
    if (!mAssets.Contains(uuid)) return nullptr;
    PLU_INFO("Found asset by UUID with name {}", mAssets[uuid].first->GetAssetName().CStr());
    return mAssets[uuid].first->GetAssetInfoPtr();
}

Plu::PathW Plu::EditorAssetManager::GetAssetPathByUUID(PluUUID uuid)
{
    return mAssets[uuid].first->GetAssetPath();
}

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::EditorAssetManager::GetAssetByPath(const PathW& path)
{
    for (std::pair asset: mAssets) {
        if (asset.second.first->GetAssetPath() == path)
            return asset.second.first;
    }
    return nullptr;
}

Plu::TypeInfo * Plu::EditorAssetManager::GetAssetViewportClass(TUsePointer<IEditorAssetObject> assetObject)
{
    String type = assetObject->mAssetType;
    for (auto handler : mAssetImporters) {
        if (handler->GetSupportedAssetType() == type) {
            auto viewportClass = handler->GetAssetViewportClass();
            if (!viewportClass) {
                String msg = "No viewport class in its handler for ";
                msg += type;
                PLU_ERROR(msg.CStr());
                break;
            }
            return viewportClass;
        }
    }
    TypeInfo* viewportClass = TypeRegistry::GetInstance()->GetTypeOfName(type + "Viewport");
    if (viewportClass) {
        return viewportClass;
    }
    return nullptr;
}

void Plu::EditorAssetManager::AddAssetFromHandler(const TOwningPointer<IEditorAssetObject>& assetObject, const PluUUID& uuid, const PathW &path, TypeInfo* type)
{
    assetObject->mAssetPath = path;
    assetObject->mAssetType = type->TypeName;
    mAssets.Insert(uuid.getUUID(), {assetObject, type});
    PLU_TRACE("Asset registered: {} of type {}", uuid.getUUID(), type->TypeName.CStr());
    PathW assetPath = path;
    DispatchEvent("NewAsset", &assetPath);
}

bool Plu::EditorAssetManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager)
{
    InitAssetManagerForPython(engineObjectManager->GetObjectAsUser<IAssetManager>(*GetEngineObjectHandle()));
    mEditorProjectManager = editorProjectManager;
    mEngineObjectManager = engineObjectManager;
    for (TypeInfo *importer: mAssetImportersTypes) {
        mAssetImporters.PushBack(DynamicCast<IEditorAssetHandler>(mEngineObjectManager->CreateObject(importer)));
    }
    PLU_PROFILE_SCOPE("Asset Load");
    bool fail = false;
    DynamicArray<PathW> assetsToLoad;
    for (const std::filesystem::directory_entry& file : std::filesystem::recursive_directory_iterator(mEditorProjectManager->GetProjectAssetsDirectory().CStr())) {
        if (file.is_directory()) continue;
        if (!file.is_regular_file()) continue;
        bool asset = file.path().extension() == PLU_ASSET_EXT_W;
        bool scn = file.path().extension() == PLU_SCENE_EXT_W;
        bool bin = file.path().extension() == PLU_BINARY_EXT_W;
        if (scn || asset || bin) {
            assetsToLoad.PushBack(file.path().wstring().c_str());
        }
    }
    for (const std::filesystem::directory_entry& file : std::filesystem::recursive_directory_iterator(EditorProjectManager::GetEngineAssetsPath().CStr())) {
        if (file.is_directory()) continue;
        if (!file.is_regular_file()) continue;
        bool asset = file.path().extension() == PLU_ASSET_EXT_W;
        bool scn = file.path().extension() == PLU_SCENE_EXT_W;
        bool bin = file.path().extension() == PLU_BINARY_EXT_W;
        if (scn || asset || bin) {
            assetsToLoad.PushBack(file.path().wstring().c_str());
        }
    }
    for (const auto& asset : assetsToLoad) {
        if (asset.GetExtension() != PLU_BINARY_EXT_W) continue;
        fail = !LoadAsset(asset.ToString());
        if (fail) {
            PLU_ERROR("Error loading asset at: {}", asset.ToString().ToNarrow().CStr());
        };
        assetsToLoad.Remove(asset);
    }
    for (const auto& asset : assetsToLoad) {
        fail = !LoadAsset(asset.ToString());
        if (fail) {
            PLU_ERROR("Error loading asset at: {}", asset.ToString().ToNarrow().CStr());
        };
    }
    DispatchEvent("LoadedAssets", nullptr);

    TypeRegistry::GetInstance()->editorAssetTUsePointerControl = [this](String name, void* value, TypeInfo* type) {
        static GameHashMap<String, DynamicArray<TUsePointer<IEditorAssetObject>>> allAssetsPerField;
        String mapKey = name + type->TypeName + reinterpret_cast<const char *>(value);
        if (!allAssetsPerField.Contains(mapKey)) {
            allAssetsPerField[mapKey] = GetAllAssetsOfType(type);
        }
        TUsePointer<IEditorAssetObject> selected = nullptr;
        for (auto i : allAssetsPerField[mapKey]) {
            if (!static_cast<TUsePointer<IAssetInfo>*>(value)->GetRaw()) continue;
            if (static_cast<TUsePointer<IAssetInfo>*>(value)->GetRaw()->Uuid == i->GetAssetInfoPtr()->Uuid) {
                selected = i;
            }
        }

        if (ImGui::Button(("Refresh##" + name).CStr()))
		{
            allAssetsPerField[mapKey] = GetAllAssetsOfType(type);
		}
		String preview = "Nothing Selected!";
		if (selected)
		{
			preview = selected->GetAssetName();
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

            for (int n = 0; n < allAssetsPerField[mapKey].Size(); n++)
            {
                PropertyInfo* nameProp = allAssetsPerField[mapKey].At(n)->GetClass()->FindProperty("Name");
                String objName;
                if (nameProp) {
                    String* name = static_cast<String *>(nameProp->GetPtr(allAssetsPerField[mapKey].At(n).GetRaw()));
                    objName = *name;
                } else {
                    objName = allAssetsPerField[mapKey].At(n)->GetAssetName();
                }
                const bool is_selected = (allAssetsPerField[mapKey].At(n) == selected);
                if (filter.PassFilter(objName.CStr()))
                    if (ImGui::Selectable(objName.CStr(), is_selected)) {
                        selected = allAssetsPerField[mapKey].At(n);
                        *static_cast<TUsePointer<IAssetInfo>*>(value) = selected->GetAssetInfoPtr();
                    }
            }
            ImGui::EndCombo();
        }
    };
    return fail;
}

bool Plu::EditorAssetManager::Shutdown()
{
    for (auto asset : mAssets) {
        if (asset.second.first->GetAssetPath().GetExtension() == PLU_BINARY_EXT_W) continue;
        if (asset.second.first->GetAssetPath().GetExtension() == PLU_SCENE_EXT_W) continue;
        for (const TOwningPointer<IEditorAssetHandler>& handler : mAssetImporters) {
            if (handler->GetSupportedAssetType() == asset.second.first->GetAssetType()) {
            }
        }
        PathW assetPath = asset.second.first->GetAssetPath();
        //assetPath += L"/" + StringW::FromNarrow(asset.second.first->GetAssetName().CStr()) + PLU_ASSET_EXT_W;
        nlohmann::json assetJson;
        assetJson = asset.second.second->SerializeToJSON(asset.second.first->GetAssetInfoPtr().GetRaw());
        DiskManager::SaveJson(assetPath.ToString(), assetJson);
        PLU_INFO("Saved asset default way");
    }
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
                break;
            }
        }
    }
}

DynamicArray<Plu::TUsePointer<Plu::IEditorAssetObject>> Plu::EditorAssetManager::GetAllAssetsOfType(TypeInfo *type)
{
    DynamicArray<TUsePointer<IEditorAssetObject>> assetsOfType;
    for (auto asset : mAssets) {
        if (asset.second.second->IsDerivedOfOrSame(type)) {
            assetsOfType.PushBack(asset.second.first);
        }
    }
    return assetsOfType;
}

void Plu::EditorAssetManager::CreateAsset(TypeInfo *assetType, const PathW &path)
{
    if (!assetType->IsDerivedOf(IAssetInfo::GetStaticClass())) return;
    mAssetCreatePath = path;
    mCurrentAssetCreationType = assetType;
    mIsCreationModalOpen = true;
}

void Plu::EditorAssetManager::HandleAssetCreationUI()
{
    static TOwningPointer<IAssetInfo> newAsset;
    //Show this monstrosity!!
    static GameHashMap<String, DynamicArray<TUsePointer<EngineObject>>> objectsPerUuidField;
    static GameHashMap<String, DynamicArray<TUsePointer<IEditorAssetObject>>> assetsPerUuidField;
    static GameHashMap<String, int> selectedObjectInUuid;
    if (mIsCreationModalOpen) {
        ImGui::OpenPopup("Asset Creator");
        mIsCreationModalOpen = false;
        void* newObj = mCurrentAssetCreationType->Construct();
        IAssetInfo* newAObj =static_cast<IAssetInfo *>(newObj);
        newAsset = TOwningPointer(newAObj);

        for (auto prop : mCurrentAssetCreationType->Properties) {
            if (prop->UuidForClass) {
                if (prop->UuidForClass->IsDerivedOfOrSame(IAssetInfo::GetStaticClass())) {
                    assetsPerUuidField[prop->PropertyName] = GetAllAssetsOfType(prop->UuidForClass);
                } else {
                    objectsPerUuidField[prop->PropertyName] = mEngineObjectManager->GetAllObjectsOfClass(prop->UuidForClass);
                }
                selectedObjectInUuid[prop->PropertyName] = -1;
            }
        }
    }
    if (mCurrentAssetCreationType && mAssetCreatePath != L"") {
        if (ImGui::BeginPopupModal("Asset Creator")) {
            static std::string assetName;
            ImGui::InputText("Asset Name", &assetName);
            for (auto prop : mCurrentAssetCreationType->Properties) {
                if (prop->UuidForClass) {
                    if (prop->UuidForClass->IsDerivedOfOrSame(IAssetInfo::GetStaticClass())) {
                        String preview;
                        if (*selectedObjectInUuid.Find(prop->PropertyName) == -1) {
                            preview = "Nothing selected!";
                        } else {
                            PropertyInfo* nameProp = assetsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName))->GetClass()->FindProperty("Name");
                            if (nameProp) {
                                preview = *static_cast<String*>(nameProp->GetPtr(assetsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName)).GetRaw()));
                            } else {
                                preview = assetsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName))->GetAssetName();
                            }
                        }
                        if (ImGui::BeginCombo(prop->PropertyName.CStr(), preview.CStr(), 0))
                        {
                            static ImGuiTextFilter filter;
                            if (ImGui::IsWindowAppearing())
                            {
                                ImGui::SetKeyboardFocusHere();
                                filter.Clear();
                            }
                            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
                            filter.Draw("##Filter", -FLT_MIN);

                            for (int n = 0; n < assetsPerUuidField.Find(prop->PropertyName)->Size(); n++)
                            {
                                PropertyInfo* nameProp = assetsPerUuidField.Find(prop->PropertyName)->At(n)->GetClass()->FindProperty("Name");
                                String objName;
                                if (nameProp) {
                                    String* name = static_cast<String *>(nameProp->GetPtr(assetsPerUuidField.Find(prop->PropertyName)->At(n).GetRaw()));
                                    objName = *name;
                                } else {
                                    objName = assetsPerUuidField.Find(prop->PropertyName)->At(n)->GetAssetName();
                                }
                                const bool is_selected = (*selectedObjectInUuid.Find(prop->PropertyName) == n);
                                if (filter.PassFilter(objName.CStr()))
                                    if (ImGui::Selectable(objName.CStr(), is_selected)) {
                                        *selectedObjectInUuid.Find(prop->PropertyName) = n;
                                        IEditorAssetObject* obj = assetsPerUuidField.Find(prop->PropertyName)->At(n).GetRaw();
                                        void* uuidPropPtr = obj->GetAssetInfoPtr()->GetClass()->GetTypeUuidProp()->GetPtr(obj->GetAssetInfoPtr().GetRaw());
                                        UInt64 uuid = static_cast<PluUUID *>(uuidPropPtr)->getUUID();
                                        *static_cast<PluUUID*>(prop->GetPtr(newAsset.GetRaw())) = uuid;
                                    }
                            }
                            ImGui::EndCombo();
                        }
                    } else {
                        String preview;
                        if (*selectedObjectInUuid.Find(prop->PropertyName) == -1) {
                            preview = "Nothing selected!";
                        } else {
                            PropertyInfo* nameProp = objectsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName))->GetClass()->FindProperty("Name");
                            if (nameProp) {
                                preview = *static_cast<String*>(nameProp->GetPtr(objectsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName)).GetRaw()));
                            } else {
                                preview = objectsPerUuidField.Find(prop->PropertyName)->At(*selectedObjectInUuid.Find(prop->PropertyName))->GetClass()->TypeName;
                            }
                        }
                        if (ImGui::BeginCombo(prop->PropertyName.CStr(), preview.CStr(), 0))
                        {
                            static ImGuiTextFilter filter;
                            if (ImGui::IsWindowAppearing())
                            {
                                ImGui::SetKeyboardFocusHere();
                                filter.Clear();
                            }
                            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
                            filter.Draw("##Filter", -FLT_MIN);

                            for (int n = 0; n < objectsPerUuidField.Find(prop->PropertyName)->Size(); n++)
                            {
                                PropertyInfo* nameProp = objectsPerUuidField.Find(prop->PropertyName)->At(n)->GetClass()->FindProperty("Name");
                                String objName;
                                if (nameProp) {
                                    String* name = static_cast<String *>(nameProp->GetPtr(objectsPerUuidField.Find(prop->PropertyName)->At(n).GetRaw()));
                                    objName = *name;
                                } else {
                                    objName = objectsPerUuidField.Find(prop->PropertyName)->At(n)->GetClass()->TypeName;
                                }
                                const bool is_selected = (*selectedObjectInUuid.Find(prop->PropertyName) == n);
                                if (filter.PassFilter(objName.CStr()))
                                    if (ImGui::Selectable(objName.CStr(), is_selected)) {
                                        *selectedObjectInUuid.Find(prop->PropertyName) = n;
                                        EngineObject* obj = objectsPerUuidField.Find(prop->PropertyName)->At(n).GetRaw();
                                        void* uuidPropPtr = obj->GetClass()->GetTypeUuidProp()->GetPtr(obj);
                                        UInt64 uuid = static_cast<PluUUID *>(uuidPropPtr)->getUUID();
                                        *static_cast<PluUUID*>(prop->GetPtr(newAsset.GetRaw())) = uuid;
                                    }
                            }
                            ImGui::EndCombo();
                        }
                    }
                } else {
                    prop->EditorControlPtr(prop->GetPtr(newAsset.GetRaw()), prop->PropertyName);
                }
            }
            std::function<void()> cleanup = [this]() {
                mCurrentAssetCreationType = nullptr;
                mAssetCreatePath = L"";
                newAsset = nullptr;
                assetName = "";
                selectedObjectInUuid.Clear();
                objectsPerUuidField.Clear();
            };
            if (ImGui::Button("Create")) {
                //TODO continue asset creation
                PathW assetPath = mEditorProjectManager->GetProjectAssetsDirectory();
                assetPath += L"/" + StringW::FromNarrow(assetName.c_str()) + PLU_ASSET_EXT_W;
                nlohmann::json assetJson;
                assetJson = mCurrentAssetCreationType->SerializeToJSON(newAsset.GetRaw());
                DiskManager::SaveJson(assetPath.ToString(), assetJson);
                LoadAsset(assetPath.ToString());
                ImGui::CloseCurrentPopup();
                cleanup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                cleanup();
            }
            ImGui::EndPopup();
        }
    }
}
