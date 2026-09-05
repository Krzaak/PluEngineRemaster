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
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Core/ApplicationInfo.h"
#include "PluEngine/Gameplay/Components/PhysicsBodyComponent.h"
#include "PluEngine/Gameplay/Components/PhysicsColliderComponent.h"
#include "PluEngine/Gameplay/Scenes/SceneWorld.h"
#include "PluEngine/Physics/JoltIntializer.h"
#include "PluEngine/Physics/PhysicsCollisionRules.h"
#include "PluEngine/Physics/PhysicsUtils.h"
#include "PluEngine/Physics/PhysicsBody.h"

void Plu::PhysicsWorld::RebuildObjectCollision(UInt64 uuid)
{
    PLU_PROFILE_SCOPE("CreatePhysicsBody");
    TUsePointer<SceneWorld> sceneWorld = mApplicationInfo->AppObjectManager->GetObjectAsUser<SceneWorld>(mSceneWorldHandle);
    TUsePointer<GameObject> gameObject = sceneWorld->GetGameObjectByUUID(uuid);

    TUsePointer<PhysicsBodyComponent> bodyComponent = gameObject->GetComponentByClass(PhysicsBodyComponent::GetStaticClass());
    DynamicArray<TUsePointer<GameObjectComponent> > colliders = gameObject->GetAllComponentsByClass(PhysicsColliderComponent::GetStaticClass());

    if (!bodyComponent || colliders.IsEmpty()) return;

    JPH::StaticCompoundShapeSettings compoundShapeSettings;

    for (auto collider : colliders) {
        TUsePointer<PhysicsColliderComponent> colliderComponent = collider;
        JPH::ShapeRefC shape = colliderComponent->GetShape();
        if (shape == nullptr) {
            PLU_CORE_ERROR("Invalid shape for collider");
            continue;
        }

        Matrix4 worldMatrix = colliderComponent->GetMatrixRelativeToGameObject();
        Vec3 loc = GetLocationFromMatrix(worldMatrix);
        Vec3 rot = GetRotationFromMatrix(worldMatrix);
        Vec3 scale = colliderComponent->GetWorldScale();

        if (scale != Vec3(1.0f)) {
            shape = new JPH::ScaledShape(shape.GetPtr(), ToJPH(scale));
        }

        compoundShapeSettings.AddShape(ToJPH(loc), ToJPHRotation(rot), shape);
    }

    JPH::Shape::ShapeResult result = compoundShapeSettings.Create();
    if (result.HasError()) {
        PLU_CORE_ERROR("Failed to create shape for collider, error {}", result.GetError());
    }
    JPH::ShapeRefC finalShape = result.Get();

    TOwningPointer<PhysicsBody> body = CreateOwning<PhysicsBody>(
        this->mPhysicsSystem->GetBodyInterface(),
        finalShape,
        ToJPH(gameObject->GetObjectLocation()),
        ToJPHRotation(gameObject->GetObjectRotation()),
        bodyComponent->Type,
        bodyComponent->Friction,
        bodyComponent->Restitution
    );

    mBodyPerObject.Insert(gameObject->GetObjectUUID(), body);

    if (!mRotLocChangesEventsPerObject.Contains(gameObject->GetObjectUUID())) {
        Int32 locEvent = gameObject->SubscribeToEvent("LocationChange", [this, gameObject](void*) {
            if (mBodyPerObject.Contains(gameObject->GetObjectUUID()) && !mIsUpdatingObjectsFromPhysics) {
                mBodyPerObject[gameObject->GetObjectUUID()]->SetPosition(ToJPH(gameObject->GetObjectLocation()));
            }
        });
        Int32 rotEvent = gameObject->SubscribeToEvent("RotationChange", [this, gameObject](void*) {
            if (mBodyPerObject.Contains(gameObject->GetObjectUUID()) && !mIsUpdatingObjectsFromPhysics) {
                mBodyPerObject[gameObject->GetObjectUUID()]->SetRotation(ToJPHRotation(gameObject->GetObjectRotation()));
            }
        });
        mRotLocChangesEventsPerObject.Insert(gameObject->GetObjectUUID(), {locEvent, rotEvent});
    }
}

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

    sceneWorld->SubscribeToEvent("NewComponent", [this](void* data) {
        TUsePointer<GameObjectComponent> newComponent = *static_cast<TUsePointer<GameObjectComponent>*>(data);
        TUsePointer<GameObject> parentObject = newComponent->GetParentGameObject();
        if (newComponent->GetClass()->IsDerivedOfOrSame(PhysicsColliderComponent::GetStaticClass())) {
            mCollidersPerObject[parentObject->GetObjectUUID()].PushBack(DynamicCast<PhysicsColliderComponent>(newComponent));
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

        if (newComponent->GetClass() == PhysicsBodyComponent::GetStaticClass()) {
            mBodyComponentPerObject[parentObject->GetObjectUUID()] = newComponent;
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());

            newComponent->SubscribeToEvent("GetLinearVelocity", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                Vec3 linearVelocity = ToGLM(mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->GetLinearVelocity());
                *static_cast<Vec3*>(data) = linearVelocity;
            });
            newComponent->SubscribeToEvent("SetLinearVelocity", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->SetLinearVelocity(ToJPH(*static_cast<Vec3*>(data)));
            });
            newComponent->SubscribeToEvent("AddLinearVelocity", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->AddLinearVelocity(ToJPH(*static_cast<Vec3*>(data)));
            });

            newComponent->SubscribeToEvent("GetAngularVelocity", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                Vec3 angularVelocity = ToGLM(mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->GetAngularVelocity());
                *static_cast<Vec3*>(data) = angularVelocity;
            });
            newComponent->SubscribeToEvent("SetAngularVelocity", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->SetAngularVelocity(ToJPH(*static_cast<Vec3*>(data)));
            });

            newComponent->SubscribeToEvent("GetFriction", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                float friction = mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->GetFriction();
                *static_cast<float*>(data) = friction;
            });
            newComponent->SubscribeToEvent("SetFriction", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->SetFriction(*static_cast<float*>(data));
            });

            newComponent->SubscribeToEvent("GetRestitution", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                float restitution = mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->GetRestitution();
                *static_cast<float*>(data) = restitution;
            });
            newComponent->SubscribeToEvent("SetRestitution", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->SetRestitution(*static_cast<float *>(data));
            });

            newComponent->SubscribeToEvent("AddForce", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->AddForce(ToJPH(*static_cast<Vec3*>(data)));
            });
            newComponent->SubscribeToEvent("AddTorque", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->AddTorque(ToJPH(*static_cast<Vec3*>(data)));
            });
            newComponent->SubscribeToEvent("AddImpulse", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->AddImpulse(ToJPH(*static_cast<Vec3*>(data)));
            });
            newComponent->SubscribeToEvent("AddAngularImpulse", [newComponent, this](void* data) {
                TUsePointer<GameObject> bodyOwnerObject = newComponent->GetParentGameObject();
                if (!mBodyPerObject.Contains(bodyOwnerObject->GetObjectUUID())) return;
                mBodyPerObject[bodyOwnerObject->GetObjectUUID()]->AddAngularImpulse(ToJPH(*static_cast<Vec3*>(data)));
            });
        }

    });

    sceneWorld->SubscribeToEvent("DestroyComponent", [this](void* data) {
        TUsePointer<GameObjectComponent> oldComponent = *static_cast<TUsePointer<GameObjectComponent>*>(data);
        TUsePointer<GameObject> parentObject = oldComponent->GetParentGameObject();
        if (oldComponent->GetClass()->IsDerivedOfOrSame(PhysicsColliderComponent::GetStaticClass())) {
            if (mCollidersPerObject.Contains(parentObject->GetObjectUUID())) {
                mCollidersPerObject[parentObject->GetObjectUUID()].Remove(oldComponent);
                if (mCollidersPerObject[parentObject->GetObjectUUID()].IsEmpty()) {
                    mCollidersPerObject.Remove(parentObject->GetObjectUUID());
                }
            }
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

        if (oldComponent->GetClass() == PhysicsBodyComponent::GetStaticClass()) {
            mBodyComponentPerObject.Remove(parentObject->GetObjectUUID());
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

    });
}

void Plu::PhysicsWorld::OnUpdate(float deltaTime)
{
    if (!mObjectsToCheck.IsEmpty()) {
        for (auto uuid : mObjectsToCheck) {
            RebuildObjectCollision(uuid);
        }
        mObjectsToCheck.Clear();
    }
    PLU_PROFILE_SCOPE("Physics Tick");
    mPhysicsSystem->Update(deltaTime, 1, mAllocator.GetRaw(), JoltPhysics::GetJoltThreadPool().GetRaw());

    mIsUpdatingObjectsFromPhysics = true;
    TUsePointer<SceneWorld> sceneWorld = mApplicationInfo->AppObjectManager->GetObjectAsUser<SceneWorld>(mSceneWorldHandle);
    for (const auto& body : mBodyPerObject) {
        TUsePointer<PhysicsBody> actualBody = body.second;
        TUsePointer<GameObject> gameObject = sceneWorld->GetGameObjectByUUID(body.first);

        gameObject->SetObjectLocation(ToGLM(actualBody->GetPosition()));

        JPH::Quat jphRot = actualBody->GetRotation();
        glm::quat glmRot(jphRot.GetW(), jphRot.GetX(), jphRot.GetY(), jphRot.GetZ());
        Vec3 eulerDeg = glm::degrees(glm::eulerAngles(glmRot));
        gameObject->SetObjectRotation(eulerDeg);
    }
    mIsUpdatingObjectsFromPhysics = false;
}
