//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_PHYSICSWORLD_H
#define PLUENGINE_PHYSICSWORLD_H

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <memory>
#include <mutex>
#include "PhysicsCollisionRules.h"
#include "CollisionChannels.h"
#include "PluSTL_FWD.h"
#include "PluEngine/PluTypes.h"
#include "PhysicsWorld.generated.h"
#include "PluEngine/Objects/EngineObject.h"

#ifdef PLU_ENGINE_EDITOR_BUILD
#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"
#endif

namespace Plu
{
	class ShaderProgram;
	class WorldComponent;
	class GameObject;
	class SceneWorld;
	class PhysicsBodyComponent;

	// Tryb wizualizacji debugowej kolizji fizyki. Ustawiany z UI edytora (main),
	// czytany przy ekstrakcji geometrii w RenderSnapshotBuilder (main).
	PLU_ENUM(PyNamespace=Plu)
	enum class PhysicsDebugRender
	{
		NONE,
		POINTS,
		WIREFRAME,
		BOTH
	};

	PLU_STRUCT(PyExport)
	struct PLU_API RaycastHit
	{
		REFLECTION_BODY_RAYCASTHIT()
	public:
		PLU_PROPERTY(PyExport)
		bool Hit = false;
		PLU_PROPERTY(PyExport)
		Vec3 HitLocation;
		PLU_PROPERTY(PyExport)
		float Fraction;
		PLU_PROPERTY(PyExport, PyReadOnly)
		GameObject* HitObject;
		JPH::BodyID PhysicsBodyHit;
	};

	PLU_STRUCT(PyExport)
	struct PLU_API RaycastDebugSettings
	{
		REFLECTION_BODY_RAYCASTDEBUGSETTINGS()
	public:
		PLU_PROPERTY(PyExport)
		bool DrawDebug = false;

		//0.0 means it will be only rendered for one frame
		//More than 0.0 means seconds of draw
		PLU_PROPERTY(PyExport)
		float DrawTime = 0.0f;
	};

	class OverlapContactListener : public JPH::ContactListener
	{
	public:
		explicit OverlapContactListener(class PhysicsWorld* world) : mWorld(world) {}

		// Channel filtering happens here (not in a GroupFilter): reject Ignore pairs early.
		JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                                      JPH::RVec3Arg inBaseOffset,
		                                      const JPH::CollideShapeResult& inCollisionResult) override;
		void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                    const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                        const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

	private:
		// Per-pair channel response. Sets ioSettings.mIsSensor for Overlap pairs (no physical
		// block) and reports whether the pair should generate overlap events.
		bool ResolveOverlap(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::ContactSettings& ioSettings) const;

		PhysicsWorld* mWorld;
	};

	PLU_CLASS(PyExport)
	class PLU_API PhysicsWorld : public EngineObject
	{
		REFLECTION_BODY_PHYSICSWORLD()
		friend class OverlapContactListener;
	private:
		GameHashMap<UInt64, TUsePointer<GameObject>> mObjectsNeedShape;
		GameHashMap<UInt32, UInt64> mBodiesPerObject;
		TUsePointer<SceneWorld> mSceneWorld;
		TUsePointer<EngineObjectManager> mEngineObjectManager;
	public:
		PhysicsWorld();
		virtual ~PhysicsWorld() override;

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void Init(TUsePointer<SceneWorld> sceneWorld, TUsePointer<EngineObjectManager> engineObjectManager);
		void NewPhysicsComponent(TUsePointer<PhysicsBodyComponent> component, bool isPlaying);
		// (Re)builds the compound shape and physics body for a game object from its current
		// components, transform and scale. Destroys any previous shape/body first.
		void RebuildGameObjectBody(GameObject* gameObject);
		void Play();
		void Shutdown();

		void Update(float DeltaTime);
		void RemoveGameObjectBodies(class GameObject* gameObject);
		// Główny: zdejmuje timery raycastów (decay o dt) i dopisuje segmenty (pos+color
		// interleaved) do bufora linii snapshotu. Zastępuje dawne GL-owe DrawDebugRaycasts.
		void CollectDebugRaycasts(float deltaTime, DynamicArray<float>& outLineVerts);
		PLU_FUNCTION()
		RaycastHit Raycast(const Vec3& Origin, const Vec3& Direction, float MaxDistance = 1000.0f, RaycastDebugSettings DebugDrawSettings = RaycastDebugSettings());

#ifdef PLU_ENGINE_EDITOR_BUILD
		void DrawEditModeShapes(JoltWireframeRenderer* wireframe, JoltPointRenderer* points,
		                        Vec3 wireColor, Vec3 pointColor);
		void InvalidateMeshCollisionCache(StaticMesh* mesh = nullptr);
#endif

		JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
		JPH::PhysicsSystem& GetSystem()        { return *mPhysicsSystem; }

		// UE-style collision channel config lives in the process-wide ActiveCollisionConfig()
		// (set from the project / ProjectDefaults). These are thin accessors over it.
		const CollisionConfig& GetCollisionConfig() const { return ActiveCollisionConfig(); }
		void SetCollisionConfig(const CollisionConfig& config) { ActiveCollisionConfig() = config; ActiveCollisionConfig().NormalizeProfiles(); }
		UInt32 ResolveCollisionProfileIndex(const String& name) const { return ActiveCollisionConfig().FindProfileIndex(name); }

		// Ustawienia wizualizacji debugowej fizyki (czytane na main przy budowie snapshotu).
		PhysicsDebugRender PhysicsDebugRenderMode      = PhysicsDebugRender::WIREFRAME;
		Vec3               PhysicsDebugRenderColorWireframe = Vec3(1, 0, 0);
		Vec3               PhysicsDebugRenderColorPoints    = Vec3(1, 0, 0);
	private:
		struct Line
		{
			Vec3 A, B;
			bool hit = false;
			Vec3 AfterHit;
		};

		DynamicArray<std::pair<float, Line>> mRaycastsToDraw;
		TOwningPointer<JPH::TempAllocatorImpl>                 mAllocator;
		TOwningPointer<JPH::JobSystemThreadPool>               mJobSystem;
		TOwningPointer<JPH::PhysicsSystem>                     mPhysicsSystem;
		TOwningPointer<BPLayerInterfaceImpl>                   mBPLayerInterface;
		TOwningPointer<ObjectVsBroadPhaseLayerFilterImpl>      mObjVsBPFilter;
		TOwningPointer<ObjectLayerPairFilterImpl>              mObjVsObjFilter;
		TOwningPointer<OverlapContactListener>                 mContactListener;

		struct PendingOverlapEvent
		{
			UInt32          BodyIdA;
			UInt32          BodyIdB;
			JPH::SubShapeID SubShapeA;
			JPH::SubShapeID SubShapeB;
			bool            IsBegin;
		};
		std::mutex                        mOverlapMutex;
		DynamicArray<PendingOverlapEvent> mPendingOverlapEvents;
		GameHashMap<UInt64, std::pair<JPH::SubShapeID, JPH::SubShapeID>> mActiveSensorPairs;

		static constexpr int cCollisionSteps = 1;

#ifdef PLU_ENGINE_EDITOR_BUILD
		GameHashMap<StaticMesh*, DynamicArray<MeshCollisionShapeEntry>> mEditModeCollisionCache;
#endif
	};
}

#endif //PLUENGINE_PHYSICSWORLD_H