//
// Created by Plutex on 2026-03-28.
//

#ifndef PLUENGINE_ASSETBROWSERPANEL_H
#define PLUENGINE_ASSETBROWSERPANEL_H

#include "Panels/EditorPanel.h"
#include "AssetBrowserPanel.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	PLU_CLASS()
	class AssetBrowserPanel : public EditorPanel
	{
		REFLECTION_BODY_ASSETBROWSERPANEL()
	public:
		AssetBrowserPanel();
		~AssetBrowserPanel() override;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	private:
		PluUUID mUUID;
		// ── filesystem walk ───────────────────────────────────────────────────────
		void WalkDirectory(const std::filesystem::path& dirPath);
		void DrawAssetItem(const PathW& path, bool isDirectory);

		// ── callbacki — implementujesz samodzielnie ───────────────────────────────
		void OnAssetClicked(const PathW& path, bool isDirectory);
		void OnAssetDoubleClicked(const PathW& path, bool isDirectory);
		void OnAssetContextMenu(const PathW& path, bool isDirectory);

		// ── state ─────────────────────────────────────────────────────────────────
		DynamicArray<TypeInfo*> mAssetTypesForCreation;
		enum class EAssetDirectory { Assets, Scripts, Shaders, EngineAssets };

		PathW               mSelectedPath;
		PathW               mCurrentPath;
		DynamicArray<PathW>  mNavigationHistory;
		EAssetDirectory     mActiveDirectory = EAssetDirectory::Assets;
	};
}

#endif //PLUENGINE_ASSETBROWSERPANEL_H