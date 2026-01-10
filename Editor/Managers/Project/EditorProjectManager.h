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
	struct EditorAppContext;
	PLU_CLASS()
	class EditorProjectManager : public EngineObject
	{
		REFLECTION_BODY_EDITORPROJECTMANAGER()
	private:
		EditorAppContext* mEditorAppContext;
		PathW mCurrentProjectPath; //Path to project
	public:
		EditorProjectManager();
		~EditorProjectManager() override;

		void SetEditorAppContext(EditorAppContext* appContext);

		[[nodiscard]] bool IsAnyProjectOpen() const;
		[[nodiscard]] StringW GetProjectDirectory() const;
		[[nodiscard]] StringW GetProjectName() const;
		[[nodiscard]]  StringW GetProjectPath() const;

		bool CreateNewProject(PathW newDirectory, const String& name);
		bool OpenProject(PathW projectPath);
		static void EnsureProjectStructure(const PathW& projectPath);

		[[nodiscard]] StringW GetProjectConfigDirectory() const;
		[[nodiscard]] StringW GetProjectAssetsDirectory() const;
	};
}

#endif //PLUENGINE_EDITORPROJECTMANAGER_H