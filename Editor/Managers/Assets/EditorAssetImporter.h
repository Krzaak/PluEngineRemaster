//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_EDITORASSETIMPORTER_H
#define PLUENGINE_EDITORASSETIMPORTER_H
#include "PluEngine/Objects/EngineObject.h"
#include "PluSTL_FWD.h"
#include "IEditorAssetHandler.generated.h"

namespace Plu
{
	class IEditorAssetObject;
	class EditorAssetManager;
	class EngineObjectManager;
	class EditorProjectManager;
	PLU_CLASS(Abstract)
	class IEditorAssetHandler : public EngineObject
	{
		REFLECTION_BODY_IEDITORASSETHANDLER()
	public:
		virtual bool ImportAsset(PathW origin, PathW loadTo) = 0;
		virtual DynamicArray<String> &GetImportableExtensions() = 0;
		virtual String GetSupportedAssetType() = 0;
		virtual TUsePointer<IEditorAssetObject> LoadAsset(
			PathW path, TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<EngineObjectManager>
			engineObjectManager, EditorAssetManager *editorAssetManager) = 0;
		virtual TypeInfo* GetAssetViewportClass() = 0;
	};
}

#endif //PLUENGINE_EDITORASSETIMPORTER_H
