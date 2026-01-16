//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_EDITORSCENESMANAGER_H
#define PLUENGINE_EDITORSCENESMANAGER_H
#include "PluEngine/Managers/ScenesManager.h"
#include "EditorScenesManager.generated.h"

namespace Plu
{
	class EditorScene;
	class EngineObjectManager;
	class EditorProjectManager;
	PLU_CLASS()
	class EditorScenesManager final : public ScenesManager
	{
		REFLECTION_BODY_EDITORSCENESMANAGER()
	private:
		TUsePointer<EditorProjectManager> mEditorProjectManager;
		TUsePointer<EngineObjectManager> mEngineObjectManager;

		HashSet<String> mSceneURLs;

		TOwningPointer<EditorScene> mActiveScene;

		bool OpenSceneInternal(String url);
	public:
		EditorScenesManager();
		~EditorScenesManager() override;

		void CreateNewScene(String name, PathW path);
		void Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager> &engineObjectManager);
		bool ConnectToWorld(String URL) override;
		String GetCurrentWorldName() override;
	};
}

#endif //PLUENGINE_EDITORSCENESMANAGER_H