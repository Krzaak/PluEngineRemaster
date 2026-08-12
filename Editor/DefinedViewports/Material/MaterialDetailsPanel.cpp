//
// Created by Plutex on 1/14/26.
//

#include "MaterialDetailsPanel.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"

#include "EditorAppContext.h"
#include "DefinedViewports/ShaderProgram/ShaderProgramInfoViewport.h"
#include "EditorViewports/EditorViewportManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetCore/AssetDescriptor.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Render/ShadersManager.h"
#include "PluEngine/AssetTypes/ShaderCode.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::MaterialDetailsPanel::GetPanelName()
{
	return "Properties";
}

void Plu::MaterialDetailsPanel::OnClosed()
{
}

void Plu::MaterialDetailsPanel::OnOpened()
{
}

void Plu::MaterialDetailsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<MaterialInfo> material = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (material)
		{
			ImGui::Text("Shader program: %s", gApplicationInfo->AppAssetManager->GetAssetDescriptor(material->shaderProgram)->AssetName.CStr());
			if (ImGui::Button("Open Shader Program Viewport")) {
				gEditorAppContext->EditorViewportManager->CreateViewport(gApplicationInfo->AppAssetManager->GetAssetPath(material->shaderProgram).ToString().ToWide(), ShaderProgramInfoViewport::GetStaticClass());
			}
			ImGui::Separator();

			for (const auto& param : material->MaterialParameters) {
				if (!param) continue;
				if (param->ArraySize != 0) continue;
				bool changed = false;
				if (param->Type == "int") {
					changed = TypeSerializer<int>::EditorControl(&dynamic_cast<ShaderUniform<int>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "float") {
					changed = TypeSerializer<float>::EditorControl(&dynamic_cast<ShaderUniform<float>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec3") {
					changed = TypeSerializer<Vec3>::EditorControl(&dynamic_cast<ShaderUniform<Vec3>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec2") {
					changed = TypeSerializer<Vec2>::EditorControl(&dynamic_cast<ShaderUniform<Vec2>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec4") {
					changed = TypeSerializer<Vec4>::EditorControl(&dynamic_cast<ShaderUniform<Vec4>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "bool") {
					changed = TypeSerializer<bool>::EditorControl(&dynamic_cast<ShaderUniform<bool>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "sampler2D") {
					changed = TypeSerializer<TUsePointer<TextureInfo>>::EditorControl(&dynamic_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(param.GetRaw())->Data, param->Name);
				}
				if (changed) {
					PanelChangedAsset();
				}
			}
		}
	}
	EndPanel();
}
