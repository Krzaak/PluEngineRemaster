//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_EDITORASSETMANAGER_H
#define PLUENGINE_EDITORASSETMANAGER_H
#include "PluEngine/Managers/AssetsManager.h"
#include "PluSTL_FWD.h"
#include "AssetHandlers/StaticMesh/StaticMeshAssetImporter.h"
#include "EditorAssetManager.generated.h"
#include "AssetHandlers/Scenes/SceneAssetHandler.h"

namespace Plu
{
	class EngineObjectManager;
	class IEditorAssetObject;
	class EditorProjectManager;

	class EditorTypeRegistry
	{
	public:
		using EditorAssetConstructor = std::function<TOwningPointer<IEditorAssetObject>(IAssetInfo*)>;
	private:
		GameHashMap<String, EditorAssetConstructor> mEditorAssetsCreators;
	public:
		static EditorTypeRegistry* GetInstance();
		void AddConstructor(String name, EditorAssetConstructor cons);
		TOwningPointer<IEditorAssetObject> ConstructAssetObject(TypeInfo* type, IAssetInfo* assetInfo);
	};

	PLU_CLASS()
	class EditorAssetManager : public IAssetManager
	{
		REFLECTION_BODY_EDITORASSETMANAGER()
	private:
		bool mIsCreationModalOpen = false;
		TUsePointer<EditorProjectManager> mEditorProjectManager;
		TUsePointer<EngineObjectManager> mEngineObjectManager;

		GameHashMap<UInt64, TOwningPointer<IEditorAssetObject>> mAssets;
		bool LoadAssetJSON(const PathW& path);

		DynamicArray<TypeInfo*> mAssetImportersTypes = {
			StaticMeshAssetHandler::GetStaticClass(),
			SceneAssetHandler::GetStaticClass()
		};
		DynamicArray<TOwningPointer<IEditorAssetHandler>> mAssetImporters;

		TypeInfo* mCurrentAssetCreationType = nullptr;
		PathW mAssetCreatePath = L"";
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		bool LoadAsset(StringW path);
		IAssetInfo *GetAssetByUUID(PluUUID uuid) override;
		TUsePointer<IEditorAssetObject> GetAssetByPath(const PathW& path);
		TypeInfo* GetAssetViewportClass(TUsePointer<IEditorAssetObject> assetObject);
		void AddAssetFromHandler(const TOwningPointer<IEditorAssetObject>& assetObject, const PluUUID& uuid, const PathW &path);
		bool Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager);
		bool Shutdown();

		void ImportAssets(DynamicArray<PathW> Assets, PathW LoadTo);
		DynamicArray<IAssetInfo *> GetAllAssetsOfType(TypeInfo *type) override;

		void CreateAsset(TypeInfo* assetType, const PathW& path);
		void HandleAssetCreationUI();
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H