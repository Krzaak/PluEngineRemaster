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
		TUsePointer<AnimationGraphViewport> viewport = DynamicCast<AnimationGraphViewport>(GetParentViewport());

		if (animationGraph && viewport) {
			ImGuiNodeEditor::SetCurrentEditor(viewport->GetNodeEditorContext());
			ImGuiNodeEditor::Begin("Animation Graph");

			// The reusable driver does everything: draw, link create/delete, add-node palette,
			// selection, layout. It dirties the asset via the callback on any mutation.
			viewport->GetNodeGraphEditor().Draw(animationGraph.GetRaw(), [this] { PanelChangedAsset(); });

			ImGuiNodeEditor::End();
			ImGuiNodeEditor::SetCurrentEditor(nullptr);
		}
	}
	EndPanel();
}
