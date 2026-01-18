//
// Created by Plutex on 1/18/26.
//

#include "EngineClassTreePanel.h"

#include "UI/IconsFontAwesome7.h"

void Plu::EngineClassTreePanel::OnUpdate(float deltaTime)
{
	ImGui::Begin( ICON_FA_TREE " Reflection Class Tree");
	for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
		ImGui::Text("%s -> %s", type.second->TypeName.CStr(), type.second->BaseType ? type.second->BaseType->TypeName.CStr() : "G U R U");
	}
	ImGui::End();
}

void Plu::EngineClassTreePanel::OnHide()
{
}

void Plu::EngineClassTreePanel::OnShow()
{
}
