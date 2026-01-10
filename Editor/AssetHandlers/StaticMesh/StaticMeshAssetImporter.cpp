//
// Created by Plutex on 1/10/26.
//

#include "StaticMeshAssetImporter.h"

bool Plu::StaticMeshAssetImporter::ImportAsset(StringW origin, StringW loadTo)
{
	PLU_INFO("Importing: {} into: {}", String::FromWide(origin.CStr()).CStr(), String::FromWide(loadTo.CStr()).CStr());
	return true;
}

DynamicArray<Plu::String> & Plu::StaticMeshAssetImporter::GetImportableExtensions()
{
	static DynamicArray<String> extensions = {".fbx", ".obj"};
	return extensions;
}
