//
// Created by Plutex on 1/10/26.
//

#include "StaticMeshAssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "AssimpLoader.h"
#include "glm/geometric.hpp"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"


bool Plu::StaticMeshAssetImporter::ImportAsset(PathW origin, PathW loadTo)
{
	PLU_INFO("Importing: {} into: {}", origin.ToString().ToNarrow().CStr(), loadTo.ToString().ToNarrow().CStr());
	auto mio = MeshImportOptions{true};
	ImportAssetStaticMeshAssimp(origin, loadTo, mio);
    //TODO
	return true;
}

DynamicArray<Plu::String> & Plu::StaticMeshAssetImporter::GetImportableExtensions()
{
	static DynamicArray<String> extensions = {".fbx", ".obj"};
	return extensions;
}
