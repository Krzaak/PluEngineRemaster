//
// Created by Plutex on 1/12/26.
//

#ifndef PLUENGINE_SCENEASSETHANDLER_H
#define PLUENGINE_SCENEASSETHANDLER_H
#include "PluEngine/Core.h"
#include "SceneAssetHandler.generated.h"
#include "Managers/Assets/EditorAssetImporter.h"

namespace Plu
{
	PLU_CLASS()
	class SceneAssetHandler : public IEditorAssetHandler
	{
		REFLECTION_BODY_SCENEASSETHANDLER()
	public:
		SceneAssetHandler() = default;
		~SceneAssetHandler() override = default;

		DynamicArray<String> &GetImportableExtensions() override;
		String GetSupportedAssetType() override;
		bool ImportAsset(PathW origin, PathW loadTo) override;
		bool LoadAsset(PathW path, TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager) override;
	};
}

#endif //PLUENGINE_SCENEASSETHANDLER_H