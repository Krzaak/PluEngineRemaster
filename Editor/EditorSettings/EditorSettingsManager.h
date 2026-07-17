//
// Created by Plutex on 5/2/26.
//

#ifndef PLUENGINE_EDITORSETTINGSMANAGER_H
#define PLUENGINE_EDITORSETTINGSMANAGER_H

#include "PluEngine/Objects/EngineObject.h"
#include "EditorSettingsManager.generated.h"


namespace Plu {
    struct EditorSettings;
    class IWindow;
    PLU_CLASS()
    class EditorSettingsManager : public EngineObject
    {
        REFLECTION_BODY_EDITORSETTINGSMANAGER()
    private:
        EditorSettings* mSettings = nullptr;
    public:
        EditorSettingsManager();
        ~EditorSettingsManager() override;

        EditorSettings* GetSettings() const;

        // Persystencja globalna (obok exe, EditorSettings.json). Load() woła się w konstruktorze;
        // Save() wołaj po każdej zmianie ustawień z panelu.
        void Load();
        void Save() const;

        // Aplikuje zapamiętane ustawienia Display do okna (VSync + tryb + rozdzielczość).
        // Wołać po IWindow::Init(), na main threadzie (np. w PluEditor::OnPostInit).
        void ApplyDisplaySettings(const TUsePointer<IWindow>& window) const;

        static TUsePointer<EditorSettingsManager> GetInstance();
    };
}


#endif //PLUENGINE_EDITORSETTINGSMANAGER_H