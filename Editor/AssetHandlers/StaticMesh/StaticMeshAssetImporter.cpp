//
// Created by Plutex on 1/10/26.
//

#include "StaticMeshAssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "AssimpLoader.h"
#include "EditorAppContext.h"
#include "StaticMeshAssimpLoader.h"
#include "DefinedViewports/StaticMesh/StaticMeshViewport.h"
#include "glm/geometric.hpp"
#include "Managers/Assets/EditorAssetManager.h"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/PluUUID.h"

extern Plu::EditorAppContext* gEditorAppContext;
bool Plu::StaticMeshAssetHandler::ImportAsset(PathW origin, PathW loadTo)
{
	PLU_INFO("Importing: {} into: {}", origin.ToString().ToNarrow().CStr(), loadTo.ToString().ToNarrow().CStr());
	StaticMeshImportProps props{};
	props.GenerateNormals = true;
	props.Merge = false;
	props.FlipUVs = false;
	props.Scale = 0.01f;
	MeshImporter::ImportStaticMesh(props, origin, loadTo);
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
	// EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<StaticMesh>>();
	// TOwningPointer<IEditorAssetObject> assetObjectTI = engineObjectManager->GetObjectAsOwner<IEditorAssetObject>(assetObject);
	// TOwningPointer<EditorAssetObject<StaticMesh>> assetObjectT = DynamicCast<EditorAssetObject<StaticMesh>>(assetObjectTI);
	// MeshImporter::LoadStaticMesh(path, assetObjectT->AssetInfo.GetRaw());
	//editorAssetManager->AddAssetFromHandler(assetObjectT, assetObjectT->AssetInfo->Uuid, path, StaticMesh::GetStaticClass());
	return nullptr;
}

Plu::TypeInfo * Plu::StaticMeshAssetHandler::GetAssetViewportClass()
{
	return StaticMeshViewport::GetStaticClass();
}
