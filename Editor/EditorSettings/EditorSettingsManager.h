//
// Created by Plutex on 5/2/26.
//

#ifndef PLUENGINE_EDITORSETTINGSMANAGER_H
#define PLUENGINE_EDITORSETTINGSMANAGER_H

#include "PluEngine/Objects/EngineObject.h"
#include "EditorSettingsManager.generated.h"


namespace Plu {
    struct EditorSettings;
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

        static TUsePointer<EditorSettingsManager> GetInstance();
    };
}


#endif //PLUENGINE_EDITORSETTINGSMANAGER_H