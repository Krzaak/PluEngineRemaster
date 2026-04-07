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
#include <memory>
#include "PhysicsCollisionRules.h"
#include "PluSTL_FWD.h"
#include "PluEngine/PluTypes.h"
#include "PhysicsWorld.generated.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
	class GameObject;
	class SceneWorld;
	class PhysicsBodyComponent;
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

	PLU_CLASS(PyExport)
	class PLU_API PhysicsWorld : public EngineObject
	{
		REFLECTION_BODY_PHYSICSWORLD()
	private:
		GameHashMap<UInt64, UInt32> mBodiesByObjects;
		GameHashMap<UInt64, TUsePointer<GameObject>> mObjectsNeedShape;
		TUsePointer<SceneWorld> mSceneWorld;
	public:
		PhysicsWorld();
		virtual ~PhysicsWorld() override;

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void Init(TUsePointer<SceneWorld> sceneWorld);
		void NewPhysicsComponent(TUsePointer<PhysicsBodyComponent> component, bool isPlaying);
		void Play();

		void Update(float DeltaTime);
		void DrawDebugRaycasts(float deltaTime, Matrix4 viewProj);
		PLU_FUNCTION()
		RaycastHit Raycast(const Vec3& Origin, const Vec3& Direction, float MaxDistance = 1000.0f, RaycastDebugSettings DebugDrawSettings = RaycastDebugSettings());

		JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
		JPH::PhysicsSystem& GetSystem()        { return *mPhysicsSystem; }
	private:
		struct Line
		{
			Vec3 A, B;
			bool hit = false;
			Vec3 AfterHit;
		};
		UInt4 mVao = 0;
		UInt4 mVbo = 0;
		UInt4 mShader = 0;

		void Init();
		void Cleanup();
		static UInt4 BuildShader();

		DynamicArray<std::pair<float, Line>> mRaycastsToDraw;
		TOwningPointer<JPH::TempAllocatorImpl>                 mAllocator;
		TOwningPointer<JPH::JobSystemThreadPool>               mJobSystem;
		TOwningPointer<JPH::PhysicsSystem>                     mPhysicsSystem;
		TOwningPointer<BPLayerInterfaceImpl>                   mBPLayerInterface;
		TOwningPointer<ObjectVsBroadPhaseLayerFilterImpl>      mObjVsBPFilter;
		TOwningPointer<ObjectLayerPairFilterImpl>              mObjVsObjFilter;

		static constexpr int cCollisionSteps = 1;
	};
}

#endif //PLUENGINE_PHYSICSWORLD_H