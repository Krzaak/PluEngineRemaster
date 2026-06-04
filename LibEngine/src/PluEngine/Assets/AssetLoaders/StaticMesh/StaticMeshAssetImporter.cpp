//
// Created by Plutex on 1/10/26.
//

#include "PluEngine/Assets/AssetLoaders/StaticMesh/StaticMeshAssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "PluEngine/Assets/AssetLoaders/StaticMesh/AssimpLoader.h"
#include "PluEngine/Assets/AssetLoaders/StaticMesh/StaticMeshAssimpLoader.h"
#include "glm/geometric.hpp"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Assets/AssetDescriptor.h"

#ifdef PLU_ENGINE_EDITOR_BUILD
DynamicArray<Plu::String> Plu::StaticMeshAssetHandler::GetSupportedImportExtensions()
{
	return {".fbx", ".obj"};
}

Plu::TypeInfo * Plu::StaticMeshAssetHandler::GetImportSettingsClass()
{
	return StaticMeshImportProps::GetStaticClass();
}

void Plu::StaticMeshAssetHandler::HandleAssetImporting(DynamicArray<Path> &assetPaths, Path outPath,
	void *importSettings, TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager)
{
	for (auto path : assetPaths) {
		MeshImporter::ImportStaticMesh(*static_cast<StaticMeshImportProps*>(importSettings), path.ToString().ToWide(), outPath.ToString().ToWide(), assetManager);
	}
}

Plu::TypeInfo * Plu::StaticMeshAssetHandler::GetAssetTypeViewportClass()
{
	return TypeRegistry::GetInstance()->GetTypeOfName("StaticMeshViewport");
}
#endif

Plu::String Plu::StaticMeshAssetHandler::GetSupportedAssetType()
{
	return "StaticMesh";
}

bool Plu::StaticMeshAssetHandler::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
	TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
	TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
	TUsePointer<IShaderManager> shaderManager)
{
	TOwningPointer<StaticMesh> staticMesh = CreateOwning<StaticMesh>();
	MeshImporter::LoadStaticMesh(assetDesc->AssetPath.ToString().ToWide(), staticMesh.GetRaw());
	*assetDataToPopulate = staticMesh;
	return true;
}
