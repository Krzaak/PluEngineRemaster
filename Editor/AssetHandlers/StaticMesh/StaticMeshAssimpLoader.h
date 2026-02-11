//
// Created by Plutex on 2026-02-07.
//

#pragma once


#include "PluSTL_FWD.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Core.h"
#include "StaticMeshAssimpLoader.generated.h"

namespace Plu
{
	PLU_STRUCT()
	struct PLU_API StaticMeshImportProps
	{
		REFLECTION_BODY_STATICMESHIMPORTPROPS()

		PLU_PROPERTY()
		bool Merge = true;

		PLU_PROPERTY()
		float Scale = 1.0f;

		PLU_PROPERTY()
		bool FlipUVs = true;

		PLU_PROPERTY()
		bool GenerateNormals = false;
	};

	namespace MeshImporter
	{
		PLU_API bool ImportStaticMesh(StaticMeshImportProps props, PathW import, PathW outDir);
		PLU_API bool LoadStaticMesh(PathW path, StaticMesh* outMesh);
		PLU_API bool SaveStaticMesh(PathW path, StaticMesh* mesh);
	}
}