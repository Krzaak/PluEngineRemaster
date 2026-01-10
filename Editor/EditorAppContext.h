//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_EDITORAPPCONTEXT_H
#define PLUENGINE_EDITORAPPCONTEXT_H
#include "PluSTL_FWD.h"

namespace Plu
{
	class EditorPanelManager;
	class EditorAssetManager;

	struct EditorAppContext
	{
		TUsePointer<EditorAssetManager> EditorAssetManager;
		TUsePointer<EditorPanelManager> EditorPanelManager;
		TUsePointer<EditorProjectManager> EditorProjectManager;
	};
}

#endif //PLUENGINE_EDITORAPPCONTEXT_H