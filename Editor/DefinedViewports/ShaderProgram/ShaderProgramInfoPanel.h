//
// Created by Plutex on 6/11/26.
//

#ifndef PLUENGINE_SHADERPROGRAMINFOPANEL_H
#define PLUENGINE_SHADERPROGRAMINFOPANEL_H

#include "ShaderProgramInfoPanel.generated.h"
#include "EditorViewports/IEditorPanel.h"

namespace Plu
{
    PLU_CLASS()
    class ShaderProgramInfoPanel : public IEditorPanel
    {
        REFLECTION_BODY_SHADERPROGRAMINFOPANEL()
    public:
        ShaderProgramInfoPanel() = default;
        ~ShaderProgramInfoPanel() override = default;

        String GetPanelName() override;
        void OnClosed() override;
        void OnOpened() override;
        void OnUpdate(float deltaTime) override;
    };
}



#endif //PLUENGINE_SHADERPROGRAMINFOPANEL_H
