//
// Created by Plutex on 5/29/26.
//

#ifndef PLUENGINE_SCENEWORLD_H
#define PLUENGINE_SCENEWORLD_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "SceneWorld.generated.h"

namespace Plu
{
	struct SceneInfo;
	class GameClient;
	class Renderer;
	class PhysicsWorld;

	PLU_CLASS(PyExport)
	class PLU_API SceneWorld final : public EngineObject
	{
		REFLECTION_BODY_SCENEWORLD()
	protected:
		GameHashMap<UInt64, TOwningPointer<GameObject>> mGameObjects;
		GameHashMap<UInt16, TUsePointer<Controller>> mControllers;
		DynamicArray<TUsePointer<GameObject>> mObjectsToBegin;
		DynamicArray<std::pair<TUsePointer<GameObject>, bool>> mObjectsToDestroy;

		TUsePointer<EngineObjectManager> mEngineObjectManager;
		TUsePointer<Renderer> mRenderer;
		TUsePointer<GameClient> mClient;
		TOwningPointer<PhysicsWorld> mPhysicsWorld;

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
		void HandleDestroy();

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
		TUsePointer<GameObject> GetGameObjectByUUID(PluUUID uuid);

		//Getters
		PLU_FUNCTION(PyExport)
		TUsePointer<GameObject> GetGameObjectOfClass(TClassPointer<GameObject> gameObjectClass);
		PLU_FUNCTION(PyExport)
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjectsOfClass(TClassPointer<GameObject> gameObjectClass);

		void JoinPlayerLocally(UInt16 playerID);

		PLU_FUNCTION(PyExport)
		PhysicsWorld* GetPhysicsWorld();
	};
}

#endif //PLUENGINE_SCENEWORLD_H
