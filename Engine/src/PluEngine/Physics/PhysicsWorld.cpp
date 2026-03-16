//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/PhysicsWorld.h"
#include <Jolt/Physics/Body/BodyManager.h>

#include "PluEngine/PluUtils.h"

using namespace Plu;

PhysicsWorld::PhysicsWorld() {
	mAllocator = CreateOwning<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
	mJobSystem = CreateOwning<JPH::JobSystemThreadPool>(
		JPH::cMaxPhysicsJobs,
		JPH::cMaxPhysicsBarriers,
		std::thread::hardware_concurrency() - 1
	);

	mBPLayerInterface = CreateOwning<BPLayerInterfaceImpl>();
	mObjVsBPFilter    = CreateOwning<ObjectVsBroadPhaseLayerFilterImpl>();
	mObjVsObjFilter   = CreateOwning<ObjectLayerPairFilterImpl>();

	mPhysicsSystem = CreateOwning<JPH::PhysicsSystem>();
	mPhysicsSystem->Init(
		1024, 0, 1024, 1024,
		*mBPLayerInterface,
		*mObjVsBPFilter,
		*mObjVsObjFilter
	);

	PLU_CORE_INFO("Physics World Created!");
}

void PhysicsWorld::Update(float DeltaTime) {
	mPhysicsSystem->Update(
		DeltaTime,
		cCollisionSteps,
		mAllocator.GetRaw(),
		mJobSystem.GetRaw()
	);
}

RaycastHit PhysicsWorld::Raycast(const Vec3 &Origin, const Vec3 &Direction, float MaxDistance)
{
	RaycastHit HitResult;

	JPH::RRayCast    Ray    { ToJPH(Origin), ToJPH(Direction) * MaxDistance };
	JPH::RayCastResult Result;

	HitResult.Hit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, Result);
	HitResult.Fraction = 0;
	HitResult.HitLocation = {0,0,0};

	if (HitResult.Hit) {
		HitResult.HitLocation = ToGLM(Ray.GetPointOnRay(Result.mFraction));
		HitResult.Fraction = Result.mFraction;
		HitResult.PhysicsBodyHit = Result.mBodyID;
	}

	return HitResult;
}
