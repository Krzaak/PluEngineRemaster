//
// Created by Plutex on 1/6/26.
//

#ifndef PLUENGINE_EDITORPROJECTMANAGER_H
#define PLUENGINE_EDITORPROJECTMANAGER_H
#include "PluEngine/Core.h"
#include "EditorProjectManager.generated.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
	PLU_CLASS()
	class EditorProjectManager : public EngineObject
	{
		REFLECTION_BODY_EDITORPROJECTMANAGER()
	public:
		EditorProjectManager();
		~EditorProjectManager() override;

	};
}

#endif //PLUENGINE_EDITORPROJECTMANAGER_H