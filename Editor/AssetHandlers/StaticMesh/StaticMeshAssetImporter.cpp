//
// Created by Plutex on 1/10/26.
//

#include "StaticMeshAssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "AssimpLoader.h"
#include "EditorAppContext.h"
#include "DefinedViewports/StaticMesh/StaticMeshViewport.h"
#include "glm/geometric.hpp"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/PluUUID.h"

extern Plu::EditorAppContext* gEditorAppContext;
bool Plu::StaticMeshAssetHandler::ImportAsset(PathW origin, PathW loadTo)
{
	PLU_INFO("Importing: {} into: {}", origin.ToString().ToNarrow().CStr(), loadTo.ToString().ToNarrow().CStr());
	auto mio = MeshImportOptions{false};
	DynamicArray<PathW> paths;
	ImportAssetStaticMeshAssimp(origin, loadTo, mio, &paths);
	for (auto asset : paths) {
		gEditorAppContext->EditorAssetManager->LoadAsset(asset.ToString());
	}
    //TODO
	return true;
}

DynamicArray<Plu::String> & Plu::StaticMeshAssetHandler::GetImportableExtensions()
{
	static DynamicArray<String> extensions = {".fbx", ".obj"};
	return extensions;
}

Plu::String Plu::StaticMeshAssetHandler::GetSupportedAssetType()
{
	return "StaticMesh";
}

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::StaticMeshAssetHandler::LoadAsset(
	PathW path, TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<
		EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager)
{
	EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<StaticMesh>>();
	TOwningPointer<IEditorAssetObject> assetObjectTI = engineObjectManager->GetObjectAsOwner<IEditorAssetObject>(assetObject);
	TOwningPointer<EditorAssetObject<StaticMesh>> assetObjectT = DynamicCast<EditorAssetObject<StaticMesh>>(assetObjectTI);
	EditorMeshData editorMeshData;
	Plu::LoadMeshBinary(path, editorMeshData);
	assetObjectT->AssetInfo->Uuid = editorMeshData.uuid;
	assetObjectT->AssetInfo->StaticMeshData.Indices = editorMeshData.Indices;
	assetObjectT->AssetInfo->StaticMeshData.Vertices = editorMeshData.Vertices;
	assetObjectT->AssetInfo->StaticMeshData.MaterialIndex = editorMeshData.MaterialIndex;
	editorAssetManager->AddAssetFromHandler(assetObjectT, editorMeshData.uuid, path, StaticMesh::GetStaticClass());
	return assetObjectT;
}

Plu::TypeInfo * Plu::StaticMeshAssetHandler::GetAssetViewportClass()
{
	return StaticMeshViewport::GetStaticClass();
}
