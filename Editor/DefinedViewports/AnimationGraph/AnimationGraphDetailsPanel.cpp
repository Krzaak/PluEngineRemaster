//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphDetailsPanel.h"

#include "AnimationGraphViewport.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Assets/EngineAssetManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;

Plu::String Plu::AnimationGraphDetailsPanel::GetPanelName()
{
	return "Details";
}

void Plu::AnimationGraphDetailsPanel::OnClosed()
{
}

void Plu::AnimationGraphDetailsPanel::OnOpened()
{
}

void Plu::AnimationGraphDetailsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<AnimationGraph> animationGraph =
			gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());

		if (animationGraph) {
			ImGui::Text("AnimationGraph details");
		} else {
			ImGui::Text("No AnimationGraph asset loaded");
		}
	}
	EndPanel();
}
