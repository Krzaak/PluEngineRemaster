//
// Created by Plutex on 2026-06-26.
//

#ifndef PLUENGINE_RENDERGPUSTATSPANEL_H
#define PLUENGINE_RENDERGPUSTATSPANEL_H
#include "Panels/EditorPanel.h"
#include "RenderGpuStatsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class RenderGpuStatsPanel : public EditorPanel
	{
		REFLECTION_BODY_RENDERGPUSTATSPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;

	private:
		void DrawOverviewTab();
		void DrawHottestAssetsTab();

		// Directional CSM diagnostics: per-cascade caster counts + a live view of one cascade
		// layer. The viewer costs a GPU image copy per frame, so it only runs while it is open
		// (mShadowViewActive) — the request is renewed every frame and cancelled on Stop.
		void DrawShadowsSection();
		int  mShadowViewLayer = 0;
		bool mShadowViewActive = false;

		// UUID assetu -> czytelna nazwa (AssetName lub nazwa pliku ze ścieżki); "<unknown>" gdy brak.
		String ResolveAssetName(UInt64 uuid);
	};
}

#endif //PLUENGINE_RENDERGPUSTATSPANEL_H
