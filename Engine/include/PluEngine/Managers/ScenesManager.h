//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SCENESMANAGER_H
#define PLUENGINE_SCENESMANAGER_H
#include "PluSTL_FWD.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/GameCore/GameMode.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "ScenesManager.generated.h"

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

	PLU_CLASS(PyExport)
	class PLU_API SceneWorld final : public EngineObject
	{
		REFLECTION_BODY_SCENEWORLD()
	protected:
		GameHashMap<UInt64, TOwningPointer<GameObject>> mGameObjects;
		GameHashMap<UInt16, TUsePointer<Controller>> mControllers;
		TUsePointer<EngineObjectManager> mEngineObjectManager;
		TUsePointer<Renderer> mRenderer;
		TUsePointer<GameClient> mClient;
		DynamicArray<TUsePointer<GameObject>> mObjectsToBegin;
		TOwningPointer<PhysicsWorld> mPhysicsWorld;
		DynamicArray<UInt64> mObjectsWithPhysics;

		TUsePointer<GameMode> mGameMode;

		bool mIsPlaying = false;

		friend void Controller::Possess(TUsePointer<Puppet> puppet);
		friend void Controller::Unpossess();
	public:
		SceneWorld() = default;
		virtual ~SceneWorld() override;

		TUsePointer<SceneInfo> Info;

		PLU_PROPERTY()
		TClassPointer<GameMode> GameModeClass = TClassPointer<GameMode>(GameMode::GetStaticClass());

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<Renderer>& renderer, const TUsePointer<GameClient>& client);

		void LoadGameObjects();
		void UnloadGameObjects();
		void Play();

		void HandleBeginPlay();

		PLU_FUNCTION(PyExport)
		TUsePointer<Controller> GetControllerByID(UInt16 playerID);

		void TickScene(float deltaTime);

		void LoadRenderables();

		void NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component);

		PLU_FUNCTION(PyExport)
		TUsePointer<GameObject> SpawnGameObject(TClassPointer<GameObject> objectClass);
		void DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay = true);
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjects();
		void GetFormattedGameObjectNames(DynamicArray<String>* result);

		//Getters
		PLU_FUNCTION(PyExport)
		TUsePointer<GameObject> GetGameObjectOfClass(TClassPointer<GameObject> gameObjectClass);
		PLU_FUNCTION(PyExport)
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjectsOfClass(TClassPointer<GameObject> gameObjectClass);

		void JoinPlayerLocally(UInt16 playerID);

		PLU_FUNCTION(PyExport)
		PhysicsWorld* GetPhysicsWorld();
	};

	PLU_CLASS(Abstract)
	class PLU_API IScenesManager : public EngineObject
	{
		REFLECTION_BODY_ISCENESMANAGER()
	public:
		void InitSceneManagerForPython(TUsePointer<IScenesManager> scenesManager);
		virtual bool ConnectToWorld(String URL) = 0; //URL can be SceneName or IP address
		virtual String GetCurrentWorldName() = 0;
		virtual TUsePointer<SceneWorld> GetCurrentWorld() = 0;
		virtual bool IsAnySceneOpen() = 0;
		virtual void TickScene(float deltaTime) = 0;
	};

	PLU_FUNCTION()
	PLU_API TUsePointer<SceneWorld> GetCurrentWorld();
}

#endif //PLUENGINE_SCENESMANAGER_H