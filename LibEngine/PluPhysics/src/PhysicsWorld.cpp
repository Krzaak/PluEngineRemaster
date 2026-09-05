//
// Created by Plutex on 9/5/26.
//

#include "PluEngine/Physics/PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Core/TempAllocator.h>

#include "PluEngine/Core/ApplicationInfo.h"
#include "PluEngine/Gameplay/Scenes/SceneWorld.h"
#include "PluEngine/Physics/JoltIntializer.h"
#include "PluEngine/Physics/PhysicsCollisionRules.h"


Plu::PhysicsWorld::PhysicsWorld()
{
    mAllocator = CreateOwning<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    mBPLayerInterface = CreateOwning<BPLayerInterfaceImpl>();
    mObjVsBPFilter = CreateOwning<ObjectVsBroadPhaseLayerFilterImpl>();
    mObjVsObjFilter = CreateOwning<ObjectLayerPairFilterImpl>();

    mPhysicsSystem = CreateOwning<JPH::PhysicsSystem>();
    mPhysicsSystem->Init(
        1024, 0, 1024, 1024,
        *mBPLayerInterface,
        *mObjVsBPFilter,
        *mObjVsObjFilter
    );

    PLU_CORE_TRACE("Physics World Intialized");
}

Plu::PhysicsWorld::~PhysicsWorld()
{
}

void Plu::PhysicsWorld::Init()
{
    TUsePointer<SceneWorld> sceneWorld = mApplicationInfo->AppObjectManager->GetObjectAsUser<SceneWorld>(mSceneWorldHandle);
    sceneWorld->SubscribeToEvent("PhysicsTick", [this](void* data) {
        float deltaTime = *static_cast<float *>(data);
        this->OnUpdate(deltaTime);
    });
}

void Plu::PhysicsWorld::OnUpdate(float deltaTime)
{
    PLU_PROFILE_SCOPE("Physics Tick");
    mPhysicsSystem->Update(deltaTime, 1, mAllocator.GetRaw(), JoltPhysics::GetJoltThreadPool().GetRaw());
}
