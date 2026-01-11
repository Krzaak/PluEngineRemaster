//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_EDITORASSETIMPORTER_H
#define PLUENGINE_EDITORASSETIMPORTER_H
#include "PluEngine/Objects/EngineObject.h"
#include "PluSTL_FWD.h"
#include "IEditorAssetImporter.generated.h"

namespace Plu
{
	PLU_CLASS(Abstract)
	class IEditorAssetImporter : public EngineObject
	{
		REFLECTION_BODY_IEDITORASSETIMPORTER()
	public:
		virtual bool ImportAsset(PathW origin, PathW loadTo) = 0;
		virtual DynamicArray<String> &GetImportableExtensions() = 0;
	};
}

#endif //PLUENGINE_EDITORASSETIMPORTER_H
