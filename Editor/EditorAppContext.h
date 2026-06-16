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

	struct EditorAppContext
	{
		TUsePointer<class EngineAssetManager> EditorAssetManager;
		TUsePointer<class EditorPanelManager> EditorPanelManager;
		TUsePointer<class EditorProjectManager> EditorProjectManager;
		TUsePointer<class SceneManager> EditorScenesManager;
		TUsePointer<class EditorViewportManager> EditorViewportManager;
		TUsePointer<class EditorShaderManager> EditorShaderManager;
		TUsePointer<class EditorPythonManager> EditorPythonManager;
		TUsePointer<class EditorWindowsManager> EditorWindowsManager;
		TUsePointer<class EditorSceneCamera> EditorSceneCamera;

		bool NewProjectPopup = false;
		bool PIEFullscreen = false;

		struct EditorAppState
		{
			EngineObjectHandle SelectedGameObject;
			EngineObjectHandle SelectedGameObjectComponent;
		} EditorState;
	};
}

#endif //PLUENGINE_EDITORAPPCONTEXT_H