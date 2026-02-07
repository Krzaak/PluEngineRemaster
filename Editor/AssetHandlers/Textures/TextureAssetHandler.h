//
// Created by Plutex on 2026-02-07.
//

#ifndef PLUENGINE_TEXTUREASSETHANDLER_H
#define PLUENGINE_TEXTUREASSETHANDLER_H
#include "PluEngine/Core.h"
#include "TextureAssetHandler.generated.h"
#include "Managers/Assets/EditorAssetImporter.h"

namespace Plu
{
	PLU_CLASS()
	class TextureAssetHandler final : public IEditorAssetHandler
	{
		REFLECTION_BODY_TEXTUREASSETHANDLER()
	public:
		TextureAssetHandler() = default;
		~TextureAssetHandler() override = default;

		TypeInfo *GetAssetViewportClass() override;
		DynamicArray<String> &GetImportableExtensions() override;
		String GetSupportedAssetType() override;
		bool ImportAsset(PathW origin, PathW loadTo) override;
		TUsePointer<IEditorAssetObject> LoadAsset(PathW path, TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager) override;
	};
}

#endif //PLUENGINE_TEXTUREASSETHANDLER_H