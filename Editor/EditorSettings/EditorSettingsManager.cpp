//
// Created by Plutex on 5/2/26.
//

#include "EditorSettingsManager.h"

#include "EditorSettings.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

Plu::EditorSettingsManager::EditorSettingsManager() {
    mSettings = new EditorSettings();
}

Plu::EditorSettingsManager::~EditorSettingsManager() {
    delete mSettings;
}

Plu::EditorSettings * Plu::EditorSettingsManager::GetSettings() const {
    return mSettings;
}

Plu::TUsePointer<Plu::EditorSettingsManager> Plu::EditorSettingsManager::GetInstance() {
    static TUsePointer<EditorSettingsManager> instance = nullptr;
    if (!instance) {
        instance = gEngineObjectManager->CreateObject(EditorSettingsManager::GetStaticClass());
    }
    return instance;
}
