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
        TUsePointer<ShaderProgram> program;
        if (!mNoShader) {
            program = gApplicationInfo->AppShaderManager->GetShaderProgram(shaderInfo->Uuid);
        }
        if (!program) {
            mNoShader = true;
            ImGui::Text("No shader program found in GPU resources, that may indicate an error during project loading, check if Vertex and Fragment shaders are valid");
            if (!mShadersChecked) {
                mVertexShaderExists = gApplicationInfo->AppShaderManager->ShaderCodeExists(shaderInfo->VertexShaderUuid);
                mFragmentShaderExists = gApplicationInfo->AppShaderManager->ShaderCodeExists(shaderInfo->FragmentShaderUuid);
                mShadersChecked = true;
            }
        }
        ImGui::BeginDisabled(!program);
        if (ImGui::Button("Recompile")) {
            // GL Recompile robi wątek renderu; z main tylko zgłaszamy żądanie.
            program->RequestRecompile();
            gEditorAppContext->EditorShaderManager->EnsureShaderInitialized(program);
            gEditorAppContext->EditorShaderManager->RefreshShaderUniforms(program);
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Uniforms")) {
            gEditorAppContext->EditorShaderManager->RefreshShaderUniforms(program);
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (!mNoShader) {
            ImGui::Text("Vertex Shader");
            TUsePointer<EditorShaderCode> vertexShader = program->GetVertexShader();
            ImGui::Text("%s", vertexShader->Name.CStr());
            ImGui::Separator();
            ImGui::Text("Fragment Shader");
            TUsePointer<EditorShaderCode> fragmentShader = program->GetFragmentShader();
            ImGui::Text("%s", fragmentShader->Name.CStr());
        } else {
            ImGui::Text("Vertex Shader UUID");
            PluUUID vertexShader = shaderInfo->VertexShaderUuid;
            if (!mVertexShaderExists) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            }
            ImGui::Text("%s", vertexShader.toString().CStr());
            if (!mVertexShaderExists) {
                ImGui::PopStyleColor();
                ImGui::SetItemTooltip("No vertex shader code with this UUID");
            }
            ImGui::Separator();
            ImGui::Text("Fragment Shader UUID");
            PluUUID fragmentShader = shaderInfo->FragmentShaderUuid;
            if (!mFragmentShaderExists) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            }
            ImGui::Text("%s", fragmentShader.toString().CStr());
            if (!mFragmentShaderExists) {
                ImGui::PopStyleColor();
                ImGui::SetItemTooltip("No fragment shader code with this UUID");
            }
        }
    }
    EndPanel();
}
