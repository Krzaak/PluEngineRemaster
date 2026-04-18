//
// Created by Plutex on 1/14/26.
//

#include "MaterialDetailsPanel.h"

#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Shaders/ShaderCode.h"

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
		EditorAssetObject<MaterialInfo>* material = dynamic_cast<EditorAssetObject<MaterialInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (material)
		{
			for (const auto& param : material->AssetInfo->MaterialParameters) {
				if (!param) continue;
				if (param->ArraySize != 0) continue;
				if (param->Type == "int") {
					TypeSerializer<int>::EditorControl(&static_cast<ShaderUniform<int>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "float") {
					TypeSerializer<float>::EditorControl(&static_cast<ShaderUniform<float>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec3") {
					TypeSerializer<Vec3>::EditorControl(&static_cast<ShaderUniform<Vec3>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec2") {
					TypeSerializer<Vec2>::EditorControl(&static_cast<ShaderUniform<Vec2>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "vec4") {
					TypeSerializer<Vec4>::EditorControl(&static_cast<ShaderUniform<Vec4>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "bool") {
					TypeSerializer<bool>::EditorControl(&static_cast<ShaderUniform<bool>*>(param.GetRaw())->Data, param->Name);
				} else if (param->Type == "sampler2D") {
					TypeSerializer<TUsePointer<TextureInfo>>::EditorControl(&static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(param.GetRaw())->Data, param->Name);
				}
			}
		}
	}
	EndPanel();
}
