//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_EDITORAPPCONTEXT_H
#define PLUENGINE_EDITORAPPCONTEXT_H
#include "PluSTL_FWD.h"

namespace Plu
{
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
	};
}

#endif //PLUENGINE_EDITORAPPCONTEXT_H