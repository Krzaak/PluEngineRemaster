//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SCENESMANAGER_H
#define PLUENGINE_SCENESMANAGER_H
#include "AssetsManager.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ScenesManager.generated.h"
#include "PluSTL_FWD.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "PluEngine/GameCore/GameMode.h"

namespace Plu
{
	class GameObjectComponent;
}

namespace Plu
{
	class Renderer;
	class GameObject;
	class EngineObjectManager;
	class GameClient;

	PLU_STRUCT()
	struct PLU_API SceneInfo : IAssetInfo
	{
		REFLECTION_BODY_SCENEINFO()

		PLU_PROPERTY()
		String URL;
	};

	PLU_CLASS()
	class PLU_API SceneWorld final : public EngineObject
	{
		REFLECTION_BODY_SCENEWORLD()
	protected:
		GameHashMap<UInt64, TOwningPointer<GameObject>> mGameObjects;
		GameHashMap<UInt16, TUsePointer<Controller>> mControllers;
		TUsePointer<EngineObjectManager> mEngineObjectManager;
		TUsePointer<Renderer> mRenderer;
		TUsePointer<GameClient> mClient;

		TUsePointer<GameMode> mGameMode;

		friend class Controller;
		bool IsKeyDown(Key key) const;
	public:
		SceneWorld() = default;
		virtual ~SceneWorld() override;

		TUsePointer<SceneInfo> Info;

		TClassPointer<GameMode> GameModeClass = TClassPointer<GameMode>(GameMode::GetStaticClass());

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<Renderer>& renderer, const TUsePointer<GameClient>& client);

		void LoadGameObjects();
		void UnloadGameObjects();
		void Play();

		TUsePointer<Controller> GetControllerByID(UInt16 playerID);

		void TickScene(float deltaTime);

		void LoadRenderables();

		void NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component);

		TUsePointer<GameObject> SpawnGameObject(TypeInfo* objectClass);
		void DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay = true);
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjects();
		void GetFormattedGameObjectNames(DynamicArray<String>* result);

		void JoinPlayerLocally(UInt16 playerID);
	};

	PLU_CLASS(Abstract)
	class PLU_API IScenesManager : public EngineObject
	{
		REFLECTION_BODY_ISCENESMANAGER()
	public:
		virtual bool ConnectToWorld(String URL) = 0; //URL can be SceneName or IP address
		virtual String GetCurrentWorldName() = 0;
		virtual TUsePointer<SceneWorld> GetCurrentWorld() = 0;
		virtual bool IsAnySceneOpen() = 0;
		virtual void TickScene(float deltaTime) = 0;
	};
}

#endif //PLUENGINE_SCENESMANAGER_H