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

		FastHashMap<MaxUInt64, TOwningPointer<IEditorAssetObject>> mAssets;
		bool LoadAsset(const StringW& path);
		bool LoadAssetJSON(const PathW& path);

		DynamicArray<TypeInfo*> mAssetImportersTypes = {
			StaticMeshAssetHandler::GetStaticClass()
		};
		DynamicArray<TOwningPointer<IEditorAssetHandler>> mAssetImporters;
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		IAssetInfo *GetAssetByUUID(PluUUID uuid) override;
		void AddAssetFromHandler(TOwningPointer<IEditorAssetObject> assetObject, const PluUUID& uuid);
		bool Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager);
		bool Shutdown();

		void ImportAssets(DynamicArray<PathW> Assets, PathW LoadTo);
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H