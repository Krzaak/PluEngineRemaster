//
// Created by Plutex on 5/4/26.
//

#include "TextureViewerPanel.h"

#include "Panels/EditorPanelManager.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/GLTexture.h"

Plu::String Plu::TextureViewerPanel::GetPanelName()
{
    if (TextureToView) {
        return ("Texture Viewer##" + (TextureToView ? TextureToView->GetDisplayName() : "")).CStr();
    } else if (FrameBufferToView) {
        return ("Texture Viewer##" + (FrameBufferToView ? FrameBufferToView->GetDisplayName() : "")).CStr();
    }
    return "Texture Viewer";
}

void Plu::TextureViewerPanel::OnHide()
{
}

void Plu::TextureViewerPanel::OnShow()
{
    SetCanClose(true);
    if (!FrameBufferToView && !TextureToView) {
        mEditorPanelManager->ClosePanel(*GetEngineObjectHandle());
    }
}

void Plu::TextureViewerPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel() && (TextureToView || FrameBufferToView)) {
        if (TextureToView) {
            ImGui::Text("W:%d, H:%d C: %d", TextureToView->GetWidth(), TextureToView->GetHeight(), TextureToView->GetChannels());
            if (ImGui::Button("Save Texture")) {
                Path path = GetSystemUserPath();
                path += "/" + TextureToView->GetDisplayName() + ".png";
                // GL readback must run on the render thread (it owns the GL context) — enqueue instead
                // of calling SaveTexture() directly from this Main-thread panel, where there is no
                // current context and glGetTexImage would silently produce an empty image.
                mApplicationInfo->AppRenderingManager->RequestTextureSave(TextureToView, path);
            }
        } else if (FrameBufferToView) {
            ImGui::Text("W:%d, H:%d Type: %s", FrameBufferToView->GetWidth(), FrameBufferToView->GetHeight(), ToString(FrameBufferToView->GetType()).CStr());
        }
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        float availW = viewportSize.x;
        float availH = viewportSize.y;

        int width = TextureToView ? TextureToView->GetWidth() : FrameBufferToView ? FrameBufferToView->GetWidth() : 0;
        int height = TextureToView ? TextureToView->GetHeight() : FrameBufferToView ? FrameBufferToView->GetHeight() : 0;

        float texAspect = static_cast<float>(width) / static_cast<float>(height);
        float availAspect = availW / availH;

        ImVec2 imageSize;

        // Dopasowanie bez zniekształcenia:
        if (availAspect > texAspect) {
            // Obszar UI jest bardziej poziomy → ograniczamy wysokość
            imageSize.y = availH;
            imageSize.x = imageSize.y * texAspect;
        } else {
            // Obszar UI jest bardziej pionowy → ograniczamy szerokość
            imageSize.x = availW;
            imageSize.y = imageSize.x / texAspect;
        }

        // Uzyskaj ID tekstury (ważne: to musi być zwykła tekstura, nie multisample!)
        GLuint texID = TextureToView ? TextureToView->GetID() : 0;
        if (!TextureToView) {
            if (FrameBufferToView->GetDepthTexture()) {
                texID = FrameBufferToView->GetDepthTexture()->GetID();
            } else if (FrameBufferToView->GetColorTexture())
            {
                texID = FrameBufferToView->GetColorTexture()->GetID();
            }
        }

        // ImGui chce "ImTextureID"
        ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

        // Uwaga: OpenGL odwraca oś Y → dlatego UV są odwrotnie.
        ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
    }
    EndPanel();
}
