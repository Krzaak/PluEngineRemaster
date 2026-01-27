//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_EDITORAPPCONTEXT_H
#define PLUENGINE_EDITORAPPCONTEXT_H
#include "PluSTL_FWD.h"
#include "PluEngine/Objects/EngineObjectHandle.h"

namespace Plu
{
	struct EngineObjectHandle;
	class EditorScenesManager;
	class EditorPanelManager;
	class EditorAssetManager;
	class EditorProjectManager;

	struct EditorAppContext
	{
		TUsePointer<EditorAssetManager> EditorAssetManager;
		TUsePointer<EditorPanelManager> EditorPanelManager;
		TUsePointer<EditorProjectManager> EditorProjectManager;
		TUsePointer<EditorScenesManager> EditorScenesManager;
		TUsePointer<class EditorViewportManager> EditorViewportManager;
		TUsePointer<class EditorShaderManager> EditorShaderManager;
		TUsePointer<class EditorPythonManager> EditorPythonManager;

		struct EditorAppState
		{
			EngineObjectHandle SelectedGameObject;
			EngineObjectHandle SelectedGameObjectComponent;
		} EditorState;
	};
}

#endif //PLUENGINE_EDITORAPPCONTEXT_H