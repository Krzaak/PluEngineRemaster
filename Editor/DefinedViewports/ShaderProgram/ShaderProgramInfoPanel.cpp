//
// Created by Plutex on 6/11/26.
//

#include "ShaderProgramInfoPanel.h"

#include "EditorInterface.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "Managers/Shaders/EditorShaderCode.h"

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
        TUsePointer<ShaderProgram> program = gApplicationInfo->AppShaderManager->GetShaderProgram(shaderInfo->Uuid);
        if (ImGui::Button("Recompile")) {
            program->Recompile();
            gEditorAppContext->EditorShaderManager->EnsureShaderInitialized(program);
            gEditorAppContext->EditorShaderManager->RefreshShaderUniforms(program);
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Uniforms")) {
            gEditorAppContext->EditorShaderManager->RefreshShaderUniforms(program);
        }
        ImGui::Separator();
        ImGui::Text("Vertex Shader");
        TUsePointer<EditorShaderCode> vertexShader = program->GetVertexShader();
        ImGui::Text("%s", vertexShader->Name.CStr());
        ImGui::Separator();
        ImGui::Text("Fragment Shader");
        TUsePointer<EditorShaderCode> fragmentShader = program->GetFragmentShader();
        ImGui::Text("%s", fragmentShader->Name.CStr());
    }
    EndPanel();
}
