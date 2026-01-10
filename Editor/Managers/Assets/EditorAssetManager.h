//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_EDITORASSETMANAGER_H
#define PLUENGINE_EDITORASSETMANAGER_H
#include "PluEngine/Managers/AssetsManager.h"
#include "EditorAssetManager.generated.h"
#include "PluSTL_FWD.h"
#include "HashMap/HashMap.h"

namespace Plu
{
	class EditorProjectManager;
	PLU_CLASS()
	class EditorAssetManager : public IAssetManager
	{
		REFLECTION_BODY_EDITORASSETMANAGER()
	private:
		TUsePointer<EditorProjectManager> mEditorProjectManager;

		GameHashMap<MaxUInt64, IAssetInfo> mAssets;
		bool LoadAsset(StringW path);
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		IAssetInfo *GetAssetByUUID(PluUUID uuid) override;
		bool Init(const TUsePointer<EditorProjectManager> &editorProjectManager);
		bool Shutdown();
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H