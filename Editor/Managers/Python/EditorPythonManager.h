//
// Created by Plutex on 2026-01-24.
//

#ifndef PLUENGINE_EDITORPYTHONMANAGER_H
#define PLUENGINE_EDITORPYTHONMANAGER_H
#include "PluEngine/Core.h"
#include "EditorPythonManager.generated.h"
#include "PluEngine/Managers/PythonManager.h"

namespace Plu
{
	PLU_CLASS()
	class EditorPythonManager final : public IPythonManager
	{
		REFLECTION_BODY_EDITORPYTHONMANAGER()
	private:
		DynamicArray<String> mUserModules;
	public:
		EditorPythonManager();
		~EditorPythonManager() override;

		void RunProjectScripts();
		void ClearProjectScripts();

		bool RunScript(PluUUID uuid) override;
		bool RunScript(PathW path, PathW workDir, String args) override;
	};
}

#endif //PLUENGINE_EDITORPYTHONMANAGER_H