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

namespace Plu
{
	struct RaycastHit {
		bool       Hit = false;
		JPH::RVec3 Point;
		float      Fraction;
		JPH::BodyID BodyID;
	};

	class PhysicsWorld {
	public:
		PhysicsWorld();
		~PhysicsWorld() = default;

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void        Update(float DeltaTime);
		RaycastHit  Raycast(const JPH::RVec3& Origin, const JPH::Vec3& Direction, float MaxDistance = 1000.0f);

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