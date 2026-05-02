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
		TUsePointer<class EditorAssetManager> EditorAssetManager;
		TUsePointer<class EditorPanelManager> EditorPanelManager;
		TUsePointer<class EditorProjectManager> EditorProjectManager;
		TUsePointer<class EditorScenesManager> EditorScenesManager;
		TUsePointer<class EditorViewportManager> EditorViewportManager;
		TUsePointer<class EditorShaderManager> EditorShaderManager;
		TUsePointer<class EditorPythonManager> EditorPythonManager;
		TUsePointer<class EditorWindowsManager> EditorWindowsManager;

		bool NewProjectPopup = false;
		bool PIEFullscreen = false;

		struct EditorAppState
		{
			EngineObjectHandle SelectedGameObject;
			EngineObjectHandle SelectedGameObjectComponent;
		} EditorState;
	};

	PLU_STRUCT()
	struct EditorSettings {
		float EditorCameraScrollWheelSpeedMultiplier = 1.0f;
		float EditorCameraPanSpeedMultiplier = 1.0f;
		float EditorCameraMoveSpeedMultiplier = 1.0f;
		float EditorCameraLookSpeedMultiplier = 1.0f;
	};
}

#endif //PLUENGINE_EDITORAPPCONTEXT_H