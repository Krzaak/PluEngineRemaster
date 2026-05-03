//
// Created by Plutex on 5/3/26.
//

#ifndef PLUENGINE_EDITORSETTINGSPANEL_H
#define PLUENGINE_EDITORSETTINGSPANEL_H
#include "Panels/EditorPanel.h"
#include "PluEngine/Core.h"
#include "EditorSettingsPanel.generated.h"


namespace Plu
{
    PLU_CLASS()
    class EditorSettingsPanel : public EditorPanel
    {
        REFLECTION_BODY_EDITORSETTINGSPANEL()
    public:
        using EditorPanel::EditorPanel;

        String GetPanelName() override;
        void OnHide() override;
        void OnShow() override;
        void OnUpdate(float deltaTime) override;
    };
}



#endif //PLUENGINE_EDITORSETTINGSPANEL_H
