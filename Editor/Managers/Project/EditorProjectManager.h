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
	private:
		StringW mCurrentProjectPath; //Path to project
	public:
		EditorProjectManager();
		~EditorProjectManager() override;

		[[nodiscard]] bool IsAnyProjectOpen() const;
		[[nodiscard]] StringW GetProjectDirectory() const;
		[[nodiscard]] StringW GetProjectName() const;
		[[nodiscard]]  StringW GetProjectPath();

		bool CreateNewProject(StringW newDirectory, String name);
		bool OpenProject(StringW projectPath);
		void EnsureProjectStructure(StringW projectPath);

	};
}

#endif //PLUENGINE_EDITORPROJECTMANAGER_H