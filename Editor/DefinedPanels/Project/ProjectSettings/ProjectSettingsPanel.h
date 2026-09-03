//
// Created by Plutex on 5/30/26.
//

#ifndef PLUENGINE_PROJECTSETTINGSPANEL_H
#define PLUENGINE_PROJECTSETTINGSPANEL_H

#include "Panels/EditorPanel.h"
#include "ProjectSettingsPanel.generated.h"

namespace Plu
{
    PLU_CLASS()
    class ProjectSettingsPanel : public EditorPanel
    {
        REFLECTION_BODY_PROJECTSETTINGSPANEL()
    public:
        using EditorPanel::EditorPanel;

        String GetPanelName() override;
        void OnHide() override;
        void OnShow() override;
        void OnUpdate(float deltaTime) override;

    private:
        // UE-style collision preset editor (operates on ActiveCollisionConfig()).
        void DrawCollisionSettings();
        int mSelectedPreset = 0; // currently edited preset in the collision section
    };
}



#endif //PLUENGINE_PROJECTSETTINGSPANEL_H
