#pragma once

#include "IEditorViewport.h"
#include "IEditorPanel.generated.h"

namespace Plu
{
    PLU_CLASS(Abstract)
    class IEditorPanel : public EngineObject
    {
        REFLECTION_BODY_IEDITORPANEL()
    private:
        TUsePointer<IEditorViewport> mEditorViewport;
        bool mCanClose = true;
        bool mIsOpen = true;
        bool mBringToFront = false;
    protected:
        void SetCanBeClosed(bool canClose) { mCanClose = canClose; }
        TUsePointer<IEditorViewport> GetParentViewport() { return mEditorViewport; }

        bool BeginPanel(ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse);
        void EndPanel();

        void PanelChangedAsset() const;
    public:
        IEditorPanel();
        void Initialize(const TUsePointer<IEditorViewport> &viewport, bool canClose);

        [[nodiscard]] bool IsOpen() const { return mIsOpen; }

        // Select/focus this panel's tab on its next Begin (mirrors IEditorViewport::mBringToFront).
        // One-shot: consumed on the next frame so the user can still switch tabs afterwards.
        void BringToFront() { mBringToFront = true; }

        virtual void OnOpened() = 0;
        virtual void OnClosed() = 0;
        virtual void OnUpdate(float deltaTime) = 0;

        virtual String GetPanelName() = 0;
        virtual String GetPanelTitle(); //GetPanelName combined with parent viewport name to keep position and docking
    };
}
