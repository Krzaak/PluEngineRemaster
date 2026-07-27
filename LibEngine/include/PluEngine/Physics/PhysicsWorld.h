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

// Not editor-only: mCollisionShapeCache feeds the play path (RebuildGameObjectBody), which exists
// in every build. Only the wireframe/point debug renderers below are editor-exclusive.
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"

#ifdef PLU_ENGINE_EDITOR_BUILD
#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include "PluEngine/Physics/PhysicsPointRenderer.h"
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
		// Per-pair channel response, resolved per sub-shape. Sets ioSettings.mIsSensor for Overlap
		// pairs (no physical block) and reports whether the pair should generate overlap events.
		bool ResolveOverlap(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                    JPH::SubShapeID sub1, JPH::SubShapeID sub2, JPH::ContactSettings& ioSettings) const;

		// Combines the two sub-shapes' material friction/restitution into ioSettings (Jolt default
		// combine: sqrt for friction, max for restitution). Falls back to body-level values when a
		// sub-shape has no PluPhysicsMaterial (e.g. mesh collision).
		void ApplyCombinedMaterial(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                          JPH::SubShapeID sub1, JPH::SubShapeID sub2, JPH::ContactSettings& ioSettings) const;

		// Resolves the UE-style collision profile index for one sub-shape: the sub-shape's
		// PluPhysicsMaterial if present, otherwise the body's CollisionGroup::GroupID fallback.
		UInt32 ResolveSubShapeProfile(const JPH::Body& body, JPH::SubShapeID subShapeID) const;

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
		// The two collision presets every generated body picks between. FindProfileIndex is a linear
		// scan over the project's presets comparing names, so resolving them once per batch instead
		// of once per body takes that scan (and two String temporaries) out of the hot path.
		//
		// Deliberately not cached beyond a single batch: the editor's preset editor mutates
		// ActiveCollisionConfig() directly (ProjectSettingsPanel) and project loading replaces it
		// wholesale, either of which shifts indices — a longer-lived cache would silently go stale.
		struct CollisionProfileIndices
		{
			UInt32 Default = 0;
			UInt32 WorldStatic = 0;
		};

		PhysicsWorld();
		virtual ~PhysicsWorld() override;

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void Init(TUsePointer<SceneWorld> sceneWorld, TUsePointer<EngineObjectManager> engineObjectManager);
		// Only marks the owning object as needing a body. The build itself is deferred to
		// FlushPendingBodies() so that everything OnSetupComponents does after AddComponent
		// (ActiveBody, capsule dimensions, transform) is already in place when the body is built.
		void NewPhysicsComponent(TUsePointer<PhysicsBodyComponent> component);
		// Builds bodies for every object marked by NewPhysicsComponent since the last flush.
		void FlushPendingBodies();
		// Marks an object whose collision geometry no longer matches its body (e.g. a component's
		// relative transform changed — sub-shape offsets are baked into the compound shape at build
		// time). The rebuild is deferred to the next FlushPendingBodies() so that dragging a value in
		// the editor costs one rebuild per frame instead of one per setter call. Objects without a
		// body yet are ignored — they get one from the normal spawn path.
		void MarkGameObjectShapeDirty(GameObject* gameObject);
		// (Re)builds the compound shape and physics body for a game object from its current
		// components, transform and scale. Destroys any previous shape/body first.
		//
		// deferAdd leaves the new body out of the physics system; the caller must then add it (see
		// FlushPendingBodies, which batches a whole scene's worth). Single rebuilds during play use
		// the default and insert immediately.
		//
		// profiles, when given, supplies the already-resolved collision preset indices; a standalone
		// rebuild passes nullptr and resolves them itself.
		void RebuildGameObjectBody(GameObject* gameObject, bool deferAdd = false, const CollisionProfileIndices* profiles = nullptr);

		// Looks the two standard presets up in the active collision config.
		[[nodiscard]] CollisionProfileIndices ResolveCommonCollisionProfiles() const;
		void Play();
		void Shutdown();

		void Update(float DeltaTime);
		void RemoveGameObjectBodies(class GameObject* gameObject);
		// Główny: zdejmuje timery raycastów (decay o dt) i dopisuje segmenty (pos+color
		// interleaved) do bufora linii snapshotu. Zastępuje dawne GL-owe DrawDebugRaycasts.
		void CollectDebugRaycasts(float deltaTime, DynamicArray<float>& outLineVerts);
		// IgnoredObjects: bodies of these game objects are skipped by the query. Needed whenever the
		// ray starts inside its own collider (Jolt treats convex shapes as solid, so it would hit
		// itself at fraction 0).
		PLU_FUNCTION()
		RaycastHit Raycast(const Vec3& Origin, const Vec3& Direction, float MaxDistance = 1000.0f, RaycastDebugSettings DebugDrawSettings = RaycastDebugSettings(), const DynamicArray<GameObject*>& IgnoredObjects = DynamicArray<GameObject*>{});

#ifdef PLU_ENGINE_EDITOR_BUILD
		void DrawEditModeShapes(JoltWireframeRenderer* wireframe, JoltPointRenderer* points,
		                        Vec3 wireColor, Vec3 pointColor);
#endif
		// Drops built collision geometry for a mesh (nullptr = all of it). Call after a mesh's
		// collision definitions change, otherwise bodies keep being built from the old shapes.
		// Not editor-only: the play-path cache exists in every build.
		void InvalidateMeshCollisionCache(StaticMesh* mesh = nullptr);

		JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
		JPH::PhysicsSystem& GetSystem()        { return *mPhysicsSystem; }

		// UE-style collision channel config lives in the process-wide ActiveCollisionConfig()
		// (set from the project / ProjectDefaults). These are thin accessors over it.
		const CollisionConfig& GetCollisionConfig() const { return ActiveCollisionConfig(); }
		void SetCollisionConfig(const CollisionConfig& config) { ActiveCollisionConfig() = config; ActiveCollisionConfig().NormalizeProfiles(); }
		UInt32 ResolveCollisionProfileIndex(const String& name) const { return ActiveCollisionConfig().FindProfileIndex(name); }

		// Ustawienia wizualizacji debugowej fizyki (czytane na main przy budowie snapshotu).
		PhysicsDebugRender PhysicsDebugRenderMode      = PhysicsDebugRender::NONE;
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

		// Inserts already-created bodies through Jolt's batch interface. Empty input is a no-op.
		void AddBodiesBatch(DynamicArray<JPH::BodyID>& bodyIds, JPH::EActivation activation);

		// Unscaled collision geometry per mesh asset, shared by the play path (RebuildGameObjectBody,
		// which scales it via JPH::ScaledShape) and the editor's debug draw (which wants it unscaled
		// as-is). One cache means one invalidation point — see InvalidateMeshCollisionCache.
		MeshCollisionShapeCache mCollisionShapeCache;
	};
}

#endif //PLUENGINE_PHYSICSWORLD_H