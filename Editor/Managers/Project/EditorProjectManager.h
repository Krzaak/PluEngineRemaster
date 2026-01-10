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

		bool CreateNewProject(StringW newDirectory, const String& name);
		bool OpenProject(StringW projectPath);
		static void EnsureProjectStructure(const StringW& projectPath);

		StringW GetProjectConfigDirectory() const;
	};
}

#endif //PLUENGINE_EDITORPROJECTMANAGER_H