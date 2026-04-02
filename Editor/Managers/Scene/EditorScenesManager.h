//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_EDITORSCENESMANAGER_H
#define PLUENGINE_EDITORSCENESMANAGER_H
#include "PluEngine/Managers/ScenesManager.h"
#include "EditorScenesManager.generated.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	class EditorSceneCamera;
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

		//Overlay is a scene that is opened next to the normal scene and is used primarily for editor viewports
		TOwningPointer<SceneWorld> mOverlayScene;
		TOwningPointer<SceneWorld> mActiveScene;
		TOwningPointer<SceneWorld> mActivePIEScene;

		bool OpenSceneInternal(const String& url, bool editor, bool pie = false, bool exitPie = false);
		friend class SceneAssetHandler;
		void AddSceneInfo(const String& name, const TUsePointer<EditorAssetObject<SceneInfo>> &sceneAsset);
		void DeserializeWorldComponent(JSON j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject);
	public:
		EditorScenesManager();
		~EditorScenesManager() override;

		void UnloadOverlayScene(bool loadBackActive = true);
		void CreateNewScene(const String& name, PathW path);
		void Init(const TUsePointer<EditorProjectManager> &editorProjectManager, const TUsePointer<EngineObjectManager> &engineObjectManager);
		void Shutdown();
		bool ConnectToWorld(String URL) override;
		bool PrepareWorldForEditor(String URL);
		String GetCurrentWorldName() override;
		TUsePointer<SceneWorld> EnterPIE();
		void ExitPIE();
		[[nodiscard]] bool IsInPIE();
		bool IsAnySceneOpen() override;
		void SaveActiveScene();
		void LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld);
		void LoadGameObjectFromJSON(TUsePointer<SceneWorld> sceneWorld, JSON j);
		void TickScene(float deltaTime) override;
		TUsePointer<SceneWorld> GetCurrentWorld() override;
		TUsePointer<SceneWorld> CreateOverlayWorld();

		TOwningPointer<EditorSceneCamera> SceneCamera;

		TUsePointer<SceneWorld> GetCurrentEditorScene();
	};
}

#endif //PLUENGINE_EDITORSCENESMANAGER_H