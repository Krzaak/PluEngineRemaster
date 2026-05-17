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
#include "AssetHandlers/Textures/TextureAssetHandler.h"

namespace Plu
{
	class EngineObjectManager;
	class IEditorAssetObject;
	class EditorProjectManager;

	class EditorTypeRegistry
	{
	public:
		using EditorAssetConstructor = std::function<TOwningPointer<IEditorAssetObject>(TOwningPointer<IAssetInfo>)>;
	private:
		GameHashMap<String, EditorAssetConstructor> mEditorAssetsCreators;
	public:
		static EditorTypeRegistry* GetInstance();
		void AddConstructor(String name, EditorAssetConstructor cons);
		TOwningPointer<IEditorAssetObject> ConstructAssetObject(TypeInfo* type, const TOwningPointer<IAssetInfo> &assetInfo);
	};

	PLU_CLASS()
	class EditorAssetManager : public IAssetManager
	{
		REFLECTION_BODY_EDITORASSETMANAGER()
	private:
		bool mIsCreationModalOpen = false;
		TUsePointer<EditorProjectManager> mEditorProjectManager;
		TUsePointer<EngineObjectManager> mEngineObjectManager;

		GameHashMap<UInt64, std::pair<TOwningPointer<IEditorAssetObject>, TypeInfo*>> mAssets;
		GameHashMap<StringW, TOwningPointer<IEditorAssetObject>> mAssetsByPath;
		bool LoadAssetJSON(const PathW& path);

		DynamicArray<TypeInfo*> mAssetImportersTypes = {
			StaticMeshAssetHandler::GetStaticClass(),
			SceneAssetHandler::GetStaticClass(),
			TextureAssetHandler::GetStaticClass()
		};
		DynamicArray<TOwningPointer<IEditorAssetHandler>> mAssetImporters;

		TypeInfo* mCurrentAssetCreationType = nullptr;
		PathW mAssetCreatePath = L"";

		bool mLoaded = false;
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		bool LoadAsset(StringW path);
		TUsePointer<IAssetInfo> GetAssetByUUID(PluUUID uuid) override;
		PathW GetAssetPathByUUID(PluUUID uuid);
		bool AssetExistsInPath(PathW path);
		TUsePointer<IEditorAssetObject> GetAssetByPath(const PathW& path);
		TypeInfo* GetAssetViewportClass(TUsePointer<IEditorAssetObject> assetObject);
		void AddAssetFromHandler(const TOwningPointer<IEditorAssetObject>& assetObject, const PluUUID& uuid, const PathW &path, TypeInfo* type);
		bool Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager>& engineObjectManager);
		bool Shutdown();

		void PrepareAssetsForDistribution();

		void GenerateProjectPythonAssetInfo();

		void ImportAssets(DynamicArray<PathW> Assets, PathW LoadTo);
		DynamicArray<TUsePointer<IEditorAssetObject>> GetAllAssetsOfType(TypeInfo *type);

		void CreateAsset(TypeInfo* assetType, const PathW& path);
		void HandleAssetCreationUI();
	};
}

#endif //PLUENGINE_EDITORASSETMANAGER_H