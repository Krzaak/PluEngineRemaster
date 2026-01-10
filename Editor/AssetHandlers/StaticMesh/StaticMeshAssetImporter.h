//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_STATICMESHASSETIMPORTER_H
#define PLUENGINE_STATICMESHASSETIMPORTER_H
#include "Managers/Assets/EditorAssetImporter.h"
#include "StaticMeshAssetImporter.generated.h"

namespace Plu
{
	PLU_CLASS()
	class StaticMeshAssetImporter : public IEditorAssetImporter
	{
		REFLECTION_BODY_STATICMESHASSETIMPORTER()
	public:
		StaticMeshAssetImporter() = default;
		virtual ~StaticMeshAssetImporter() override = default;

		bool ImportAsset(StringW origin, StringW loadTo) override;
		DynamicArray<String> &GetImportableExtensions() override;
	};
}

#endif //PLUENGINE_STATICMESHASSETIMPORTER_H