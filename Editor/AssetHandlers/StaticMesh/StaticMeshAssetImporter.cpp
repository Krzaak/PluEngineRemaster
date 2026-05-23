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
#include "PluEngine/Assets/AssetDescriptor.h"

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

bool Plu::StaticMeshAssetHandler::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
	TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
	TUsePointer<EngineObjectManager> objectManager, TUsePointer<IScenesManager> sceneManager,
	TUsePointer<IShaderManager> shaderManager)
{
	TOwningPointer<StaticMesh> staticMesh = CreateOwning<StaticMesh>();
	MeshImporter::LoadStaticMesh(assetDesc->AssetPath.ToString().ToWide(), staticMesh.GetRaw());
	*assetDataToPopulate = staticMesh;
	return true;
}

Plu::TypeInfo * Plu::StaticMeshAssetHandler::GetAssetTypeViewportClass()
{
	return StaticMeshViewport::GetStaticClass();
}
