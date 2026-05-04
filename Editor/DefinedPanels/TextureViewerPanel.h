//
// Created by Plutex on 5/4/26.
//

#ifndef PLUENGINE_TEXTUREVIEWERPANEL_H
#define PLUENGINE_TEXTUREVIEWERPANEL_H
#include "Panels/EditorPanel.h"
#include "PluEngine/Core.h"
#include "TextureViewerPanel.generated.h"


namespace Plu
{
    class Texture;

    PLU_CLASS()
    class TextureViewerPanel : public EditorPanel
    {
        REFLECTION_BODY_TEXTUREVIEWERPANEL()
    public:
        using EditorPanel::EditorPanel;

        TUsePointer<Texture> TextureToView;

        String GetPanelName() override;
        void OnHide() override;
        void OnShow() override;
        void OnUpdate(float deltaTime) override;
    };
}



#endif //PLUENGINE_TEXTUREVIEWERPANEL_H
