//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphViewportPanel.h"

#include "AnimationGraphViewport.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Assets/EngineAssetManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;

Plu::String Plu::AnimationGraphViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::AnimationGraphViewportPanel::OnClosed()
{
}

void Plu::AnimationGraphViewportPanel::OnOpened()
{
}

void Plu::AnimationGraphViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<AnimationGraph> animationGraph =
			gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());

		if (animationGraph) {
			ImGui::Text("AnimationGraph node canvas");
		} else {
			ImGui::Text("No AnimationGraph asset loaded");
		}
	}
	EndPanel();
}
