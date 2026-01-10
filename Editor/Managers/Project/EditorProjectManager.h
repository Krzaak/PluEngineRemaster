//
// Created by Plutex on 1/6/26.
//

#ifndef PLUENGINE_EDITORPROJECTMANAGER_H
#define PLUENGINE_EDITORPROJECTMANAGER_H
#include "PluEngine/Core.h"
#include "EditorProjectManager.generated.h"
#include "Path/Path.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
	struct ApplicationInfo;
	struct EditorAppContext;
	PLU_CLASS()
	class EditorProjectManager : public EngineObject
	{
		REFLECTION_BODY_EDITORPROJECTMANAGER()
	private:
		EditorAppContext* mEditorAppContext;
		ApplicationInfo* mApplicationInfo;
		PathW mCurrentProjectPath; //Path to project
	public:
		EditorProjectManager();
		~EditorProjectManager() override;

		void SetEditorAppContext(EditorAppContext* appContext, ApplicationInfo* applicationInfo);

		[[nodiscard]] bool IsAnyProjectOpen() const;
		[[nodiscard]] PathW GetProjectDirectory() const;
		[[nodiscard]] StringW GetProjectName() const;
		[[nodiscard]]  PathW GetProjectPath() const;

		bool CreateNewProject(PathW newDirectory, const String& name);
		bool OpenProject(PathW projectPath);
		static void EnsureProjectStructure(const PathW& projectPath);

		[[nodiscard]] PathW GetProjectConfigDirectory() const;
		[[nodiscard]] PathW GetProjectAssetsDirectory() const;
		[[nodiscard]] PathW GetProjectScriptsDirectory() const;
		[[nodiscard]] PathW GetProjectShadersDirectory() const;

	};
}

#endif //PLUENGINE_EDITORPROJECTMANAGER_H