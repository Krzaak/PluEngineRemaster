//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_EDITORSCENESMANAGER_H
#define PLUENGINE_EDITORSCENESMANAGER_H
#include "PluEngine/Managers/ScenesManager.h"
#include "EditorScenesManager.generated.h"
#include "Managers/Assets/EditorAssetObject.h"

namespace Plu
{
	class IEditorAssetObject;
	class SceneWorld;
	class EngineObjectManager;
	class EditorProjectManager;
	PLU_CLASS()
	class EditorScenesManager final : public IScenesManager
	{
		REFLECTION_BODY_EDITORSCENESMANAGER()
	private:
		TUsePointer<EditorProjectManager> mEditorProjectManager;
		TUsePointer<EngineObjectManager> mEngineObjectManager;

		//HashSet<String> mSceneURLs;
		GameHashMap<String, TUsePointer<EditorAssetObject<SceneInfo>>> mRegisteredScenes;

		TOwningPointer<SceneWorld> mActiveScene;

		bool OpenSceneInternal(const String& url, bool editor);
		friend class SceneAssetHandler;
		void AddSceneInfo(const String& name, const TUsePointer<EditorAssetObject<SceneInfo>> &sceneAsset);
	public:
		EditorScenesManager();
		~EditorScenesManager() override;

		void CreateNewScene(const String& name, PathW path);
		void Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager> &engineObjectManager);
		void Shutdown();
		bool ConnectToWorld(String URL) override;
		bool PrepareWorldForEditor(String URL);
		String GetCurrentWorldName() override;
		bool IsAnySceneOpen() override;

		TUsePointer<SceneWorld> GetCurrentEditorScene();
	};
}

#endif //PLUENGINE_EDITORSCENESMANAGER_H