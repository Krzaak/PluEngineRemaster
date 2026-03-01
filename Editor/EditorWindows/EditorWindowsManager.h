//
// Created by Plutex on 2026-02-20.
//

#ifndef PLUENGINE_EDITORWINDOWSMANAGER_H
#define PLUENGINE_EDITORWINDOWSMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "EditorWindowsManager.generated.h"

namespace Plu
{
	class IWindow;
	PLU_CLASS()
	class EditorWindowsManager : public EngineObject
	{
		REFLECTION_BODY_EDITORWINDOWSMANAGER()
	public:
		EditorWindowsManager() = default;
		~EditorWindowsManager() override;

		void OnUpdate(float deltaTime);
	};
}

#endif //PLUENGINE_EDITORWINDOWSMANAGER_H