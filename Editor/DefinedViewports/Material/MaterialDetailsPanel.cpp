//
// Created by Plutex on 1/14/26.
//

#include "MaterialDetailsPanel.h"

#include "EditorAppContext.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Shaders/ShaderCode.h"

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
			for (const auto& param : material->MaterialParameters) {
				if (!param) continue;
				if (param->ArraySize != 0) continue;
				bool changed = false;
				if (param->Type == "int") {
					changed = TypeSerializer<int>::EditorControl(&static_cast<ShaderUniform<int>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "float") {
					changed = TypeSerializer<float>::EditorControl(&static_cast<ShaderUniform<float>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec3") {
					changed = TypeSerializer<Vec3>::EditorControl(&static_cast<ShaderUniform<Vec3>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec2") {
					changed = TypeSerializer<Vec2>::EditorControl(&static_cast<ShaderUniform<Vec2>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec4") {
					changed = TypeSerializer<Vec4>::EditorControl(&static_cast<ShaderUniform<Vec4>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "bool") {
					changed = TypeSerializer<bool>::EditorControl(&static_cast<ShaderUniform<bool>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "sampler2D") {
					changed = TypeSerializer<TUsePointer<TextureInfo>>::EditorControl(&static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(param.GetRaw())->Data, param->Name);
				}
				if (changed) {
					PanelChangedAsset();
				}
			}
		}
	}
	EndPanel();
}
