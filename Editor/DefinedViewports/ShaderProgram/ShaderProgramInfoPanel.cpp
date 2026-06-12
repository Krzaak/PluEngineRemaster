//
// Created by Plutex on 6/11/26.
//

#include "ShaderProgramInfoPanel.h"

#include "EditorInterface.h"
#include "PluEngine/Shaders/ShaderProgram.h"

Plu::String Plu::ShaderProgramInfoPanel::GetPanelName()
{
    return "Info";
}

void Plu::ShaderProgramInfoPanel::OnClosed()
{
}

void Plu::ShaderProgramInfoPanel::OnOpened()
{
}

void Plu::ShaderProgramInfoPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel())
    {
        TUsePointer<ShaderProgramInfo> shaderInfo = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
        if (ImGui::Button("Recompile")) {
            gApplicationInfo->AppShaderManager->GetShaderProgram(shaderInfo->Uuid)->Recompile();
        }

    }
    EndPanel();
}
