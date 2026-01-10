//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_EDITORASSETMANAGER_H
#define PLUENGINE_EDITORASSETMANAGER_H
#include "PluEngine/Managers/AssetsManager.h"
#include "PluSTL_FWD.h"
#include "AssetHandlers/StaticMesh/StaticMeshAssetImporter.h"
#include "EditorAssetManager.generated.h"

namespace Plu
{
	class EngineObjectManager;
	class IEditorAssetObject;
	class EditorProjectManager;

	PLU_CLASS()
	class EditorAssetManager : public IAssetManager
	{
		REFLECTION_BODY_EDITORASSETMANAGER()
	private:
		TUsePointer<EditorProjectManager> mEditorProjectManager;
		TUsePointer<EngineObjectManager> mEngineObjectManager;

		FastHashMap<MaxUInt64, IEditorAssetObject> mAssets;
		bool LoadAsset(StringW path);

		DynamicArray<TypeInfo*> mAssetImportersTypes = {
			StaticMeshAssetImporter::GetStaticClass()
		};
		DynamicArray<TOwningPointer<IEditorAssetImporter>> mAssetImporters;
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		IAssetInfo *GetAssetByUUID(PluUUID uuid) override;
		bool Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager);
		bool Shutdown();
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H