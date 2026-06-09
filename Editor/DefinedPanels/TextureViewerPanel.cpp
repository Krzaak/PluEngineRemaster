//
// Created by Plutex on 5/4/26.
//

#include "TextureViewerPanel.h"

#include "PluEngine/PluUtils.h"
#include "PluEngine/Renderer/GLTexture.h"

Plu::String Plu::TextureViewerPanel::GetPanelName()
{
    return ("Texture Viewer##" + (TextureToView ? TextureToView->GetDisplayName() : "")).CStr();
}

void Plu::TextureViewerPanel::OnHide()
{
}

void Plu::TextureViewerPanel::OnShow()
{
    SetCanClose(true);
}

void Plu::TextureViewerPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel() && TextureToView) {
        ImGui::Text("W:%d, H:%d C: %d", TextureToView->GetWidth(), TextureToView->GetHeight(), TextureToView->GetChannels());
        if (ImGui::Button("Save Texture")) {
            Path path = GetSystemUserPath();
            path += "/" + TextureToView->GetDisplayName() + ".png";
            TextureToView->SaveTexture(path);
        }
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        float availW = viewportSize.x;
        float availH = viewportSize.y;

        float texAspect = (float)TextureToView->GetWidth() / (float)TextureToView->GetHeight();
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
        GLuint texID = TextureToView->GetID();

        // ImGui chce "ImTextureID"
        ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

        // Uwaga: OpenGL odwraca oś Y → dlatego UV są odwrotnie.
        ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
    }
    EndPanel();
}
