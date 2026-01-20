//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_STATICMESHASSETIMPORTER_H
#define PLUENGINE_STATICMESHASSETIMPORTER_H
#include "Managers/Assets/EditorAssetImporter.h"
#include "StaticMeshAssetImporter.generated.h"
#include "Path/Path.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

namespace Plu
{
	struct EditorMeshData : MeshData
	{
		Plu::String Name;
		PluUUID uuid;
	};

	struct MeshImportOptions
	{
		bool MergeMeshes = true;
	};

	PLU_CLASS()
	class StaticMeshAssetHandler : public IEditorAssetHandler
	{
		REFLECTION_BODY_STATICMESHASSETHANDLER()
	public:
		StaticMeshAssetHandler() = default;
		virtual ~StaticMeshAssetHandler() override = default;

		bool ImportAsset(PathW origin, PathW loadTo) override;
		DynamicArray<String> &GetImportableExtensions() override;
		String GetSupportedAssetType() override;
		TUsePointer<IEditorAssetObject> LoadAsset(
			PathW path, TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<
				EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager) override;
		TypeInfo *GetAssetViewportClass() override;
	};
}

#endif //PLUENGINE_STATICMESHASSETIMPORTER_H