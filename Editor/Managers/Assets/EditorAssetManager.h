//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_EDITORASSETMANAGER_H
#define PLUENGINE_EDITORASSETMANAGER_H
#include "PluEngine/Managers/AssetsManager.h"
#include "EditorAssetManager.generated.h"

namespace Plu
{
	PLU_CLASS()
	class EditorAssetManager : public IAssetManager
	{
		REFLECTION_BODY_EDITORASSETMANAGER()
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		IAssetInfo *GetAssetByUUID(PluUUID uuid) override;
		bool Init(StringW PathToProject) override;
		bool Shutdown() override;
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H