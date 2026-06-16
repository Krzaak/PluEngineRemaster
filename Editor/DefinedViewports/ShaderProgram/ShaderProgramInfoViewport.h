//
// Created by Plutex on 6/11/26.
//

#ifndef PLUENGINE_SHADERPROGRAMINFOVIEWPORT_H
#define PLUENGINE_SHADERPROGRAMINFOVIEWPORT_H

#include "ShaderProgramInfoViewport.generated.h"
#include "EditorViewports/IEditorViewport.h"

namespace Plu
{
    PLU_CLASS()
    class ShaderProgramInfoViewport : public IEditorViewport
    {
        REFLECTION_BODY_SHADERPROGRAMINFOVIEWPORT()
    public:
        ShaderProgramInfoViewport() = default;
        ~ShaderProgramInfoViewport() override = default;

        void OnClosed() override;
        void OnOpened() override;
        void OnPanelRegister() override;
        void OnUpdate(float deltaTime) override;
    };
}



#endif //PLUENGINE_SHADERPROGRAMINFOVIEWPORT_H
