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

	PLU_CLASS(PyExport)
	class PhysicsWorld : public EngineObject
	{
		REFLECTION_BODY_PHYSICSWORLD()
	public:
		PhysicsWorld();
		~PhysicsWorld() = default;

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void Update(float DeltaTime);
		PLU_FUNCTION()
		RaycastHit Raycast(const Vec3& Origin, const Vec3& Direction, float MaxDistance = 1000.0f);

		JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
		JPH::PhysicsSystem& GetSystem()        { return *mPhysicsSystem; }
	private:
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