//
// Created by Plutex on 5/29/26.
//

#ifndef PLUENGINE_SCENEWORLD_H
#define PLUENGINE_SCENEWORLD_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "SceneWorld.generated.h"
#include "PluEngine/GameCore/Controller.h"
#include "PluEngine/GameCore/GameMode.h"
#include "PluEngine/Renderer/RenderSnapshotBuilder.h"

namespace Plu
{
	class CameraComponent;
	class DirectionalLight;
	class StaticMeshComponent;
	class InstancedStaticMeshComponent;
	class SkeletalMeshComponent;
	class IRenderable;
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
		// Objects spawned while the tick loop below is running. Inserting into mGameObjects
		// mid-iteration rehashes the map and invalidates the iterator, so they wait here until the
		// loop ends. Empty outside of the tick loop.
		DynamicArray<TOwningPointer<GameObject>> mPendingSpawns;
		bool mTickingGameObjects = false;
		DynamicArray<std::pair<TUsePointer<GameObject>, bool>> mObjectsToDestroy;

		TUsePointer<EngineObjectManager> mEngineObjectManager;
		TUsePointer<GameClient> mClient;
		TOwningPointer<PhysicsWorld> mPhysicsWorld;

		TUsePointer<GameMode> mGameMode;

		//Renderables
		GameHashMap<UInt64, DynamicArray<TOwningPointer<StaticMeshComponent>>> mStaticMeshRenderables;
		GameHashMap<UInt64, DynamicArray<TOwningPointer<InstancedStaticMeshComponent>>> mInstancedMeshRenderables;
		GameHashMap<UInt64, DynamicArray<TOwningPointer<SkeletalMeshComponent>>> mSkeletalMeshRenderables;
		TOwningPointer<DirectionalLight> mDirectionalLight;

		bool mIsPlaying = false;
		bool mNewGameObjectSpawned = false;

		// Per-world cache for GetAllGameObjectsOfClass. Was a function-local static
		// (globally shared across worlds + not thread-safe); kept per-world here.
		GameHashMap<String, DynamicArray<TUsePointer<GameObject>>> mGameObjectsPerClassCache;

		friend void Controller::Possess(TUsePointer<Puppet> puppet);
		friend void Controller::Unpossess();
		friend void RenderSnapshotBuilder::BuildSnapshotAndPublish(float deltaTime);
	public:
		SceneWorld() = default;
		virtual ~SceneWorld() override;

		TUsePointer<SceneInfo> Info;

		PLU_PROPERTY()
		TClassPointer<GameMode> GameModeClass = TClassPointer<GameMode>(GameMode::GetStaticClass());

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<GameClient>& client);

		void LoadGameObjects();
		void UnloadGameObjects();
		void Play();

		void HandleBeginPlay();
		void HandleDestroy();
		// Moves objects spawned during the tick loop into mGameObjects. Until this runs they are
		// not visible to GetGameObjectByUUID / GetAllGameObjects*.
		void FlushPendingSpawns();

		PLU_FUNCTION(PyExport)
		TUsePointer<Controller> GetControllerByID(UInt16 playerID);

		void TickScene(float deltaTime);

		void NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component);

		// Called when a game object's scale changes. While playing, this rebuilds the object's
		// physics body so its colliders match the new scale (Jolt shapes can't be scaled in place).
		void OnGameObjectScaleChanged(GameObject* gameObject);

		PLU_FUNCTION(PyExport)
		TUsePointer<GameObject> SpawnGameObject(TClassPointer<GameObject> objectClass);
		void DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay = true);

		PLU_FUNCTION(PyExport)
		void DestroyGameObject(GameObject* gameObject);

		DynamicArray<TUsePointer<GameObject>> GetAllGameObjects();
		void GetFormattedGameObjectNames(DynamicArray<String>* result);

		/**
		 * Domyślna nazwa dla nowego obiektu: `TypeName` + **najniższy wolny** indeks w tej scenie
		 * (np. `StaticMeshActor0`, potem `StaticMeshActor1`). Numeracja jest lokalna dla sceny i
		 * liczona od stanu faktycznego, więc kasowanie obiektów zwalnia numerki zamiast je zawyżać.
		 */
		String MakeDefaultObjectName(TClassPointer<GameObject> objectClass);
		/**
		 * To samo, ale dla dowolnego prefiksu zamiast `TypeName` — dla obiektów, które mają już
		 * nadaną ręcznie nazwę (duplikat `Tree3` dostaje `Tree4`, a nie `StaticMeshActor7`).
		 */
		String MakeDefaultObjectNameFromBase(const String& base);
		/** Czy jakiś obiekt w scenie (łącznie z pending spawns) nosi już taką nazwę. */
		[[nodiscard]] bool IsObjectNameTaken(const String& name) const;
		TUsePointer<GameObject> GetGameObjectByUUID(PluUUID uuid);

		//Getters
		PLU_FUNCTION(PyExport)
		TUsePointer<GameObject> GetGameObjectOfClass(TClassPointer<GameObject> gameObjectClass);
		PLU_FUNCTION(PyExport)
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjectsOfClass(TClassPointer<GameObject> gameObjectClass);

		void JoinPlayerLocally(UInt16 playerID);

		PLU_FUNCTION(PyExport)
		PhysicsWorld* GetPhysicsWorld() const;
	};
}

#endif //PLUENGINE_SCENEWORLD_H
