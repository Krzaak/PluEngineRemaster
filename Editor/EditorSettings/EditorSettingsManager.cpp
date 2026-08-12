//
// Created by Plutex on 5/2/26.
//

#include "EditorSettingsManager.h"

#include "EditorSettings.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Platform/Window.h"
#include "PluEngine/Platform/DiskManager.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"
#include "PluEngine/Core/Reflection/TypeTraits.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

namespace
{
    Plu::PathW GetSettingsJSONPath()
    {
        return Plu::GetExePath().GetParentPath() / L"EditorSettings.json";
    }
}

Plu::EditorSettingsManager::EditorSettingsManager() {
    mSettings = new EditorSettings();
    Load();
}

Plu::EditorSettingsManager::~EditorSettingsManager() {
    delete mSettings;
}

Plu::EditorSettings * Plu::EditorSettingsManager::GetSettings() const {
    return mSettings;
}

void Plu::EditorSettingsManager::Load() {
    std::optional<nlohmann::json> json = DiskManager::LoadJson(GetSettingsJSONPath());
    if (!json.has_value()) return;

    // Prymitywy/enumy nie sięgają do kontekstu, ale API deserializacji go wymaga.
    DeserializationContext dc;
    TypeSerializer<TypeInfo*>::Deserialize(&dc, json.value(), EditorSettings::GetStaticClass(), mSettings);
}

void Plu::EditorSettingsManager::Save() const {
    nlohmann::json json = EditorSettings::GetStaticClass()->SerializeToJSON(mSettings);
    DiskManager::SaveJson(GetSettingsJSONPath().ToString(), json);
}

void Plu::EditorSettingsManager::ApplyDisplaySettings(const TUsePointer<IWindow>& window) const {
    if (!window) return;

    window->SetVSyncEnabled(mSettings->VSync);

    // Windowed to stan startowy okna — nie wołamy MakeFullscreen niepotrzebnie.
    if (mSettings->WindowMode != FullscreenType::Windowed) {
        window->MakeFullscreen(mSettings->WindowMode, IVec2(mSettings->FullscreenWidth, mSettings->FullscreenHeight));
    }
}

Plu::TUsePointer<Plu::EditorSettingsManager> Plu::EditorSettingsManager::GetInstance() {
    static TUsePointer<EditorSettingsManager> instance = nullptr;
    if (!instance) {
        instance = gEngineObjectManager->CreateObject(EditorSettingsManager::GetStaticClass());
    }
    return instance;
}
