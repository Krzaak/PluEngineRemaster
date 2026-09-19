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
#include "PluEngine/Gameplay/Components/StaticMeshComponent.h"
#include "PluEngine/Gameplay/Scenes/SceneWorld.h"
#include "PluEngine/Physics/JoltIntializer.h"
#include "PluEngine/Physics/PhysicsCollisionRules.h"
#include "PluEngine/Physics/PhysicsUtils.h"
#include "PluEngine/Physics/PhysicsBody.h"
#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include "PluEngine/Physics/StaticMeshCollision.h"

void Plu::PhysicsWorld::RebuildObjectCollision(UInt64 uuid)
{
    PLU_PROFILE_SCOPE("CreatePhysicsBody");
    TUsePointer<SceneWorld> sceneWorld = mApplicationInfo->AppObjectManager->GetObjectAsUser<SceneWorld>(mSceneWorldHandle);
    TUsePointer<GameObject> gameObject = sceneWorld->GetGameObjectByUUID(uuid);

    if (!gameObject) {
        if (mBodyPerObject.Contains(uuid)) {
            mBodyPerObject.Remove(uuid);
        }
        return;
    }

    TUsePointer<PhysicsBodyComponent> bodyComponent = gameObject->GetComponentByClass(PhysicsBodyComponent::GetStaticClass());
    DynamicArray<TUsePointer<GameObjectComponent>> colliders = gameObject->GetAllComponentsByClass(PhysicsColliderComponent::GetStaticClass());
    DynamicArray<TUsePointer<GameObjectComponent>> staticMeshColliders = gameObject->GetAllComponentsByClass(StaticMeshComponent::GetStaticClass());

    if (!bodyComponent || (colliders.IsEmpty() && staticMeshColliders.IsEmpty())) return;

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
            static HashMap<void*, HashMap<Vec3, JPH::ShapeRefC>> shapeCache;
            if (shapeCache.Contains(const_cast<JPH::Shape *>(shape.GetPtr()))) {
                auto shapesByScale = shapeCache.Find(const_cast<JPH::Shape *>(shape.GetPtr()));
                if (!shapesByScale->Contains(scale)) {
                    //PLU_CORE_TRACE("Cache Miss: Shape Scale");
                    shapesByScale->Insert(scale, new JPH::ScaledShape(shape.GetPtr(), ToJPH(scale)));
                }
            } else {
                shapeCache[const_cast<JPH::Shape *>(shape.GetPtr())][scale] = new JPH::ScaledShape(shape.GetPtr(), ToJPH(scale));
                //PLU_CORE_TRACE("Cache Miss: Shape Ptr {}", static_cast<void*>(const_cast<JPH::Shape *>(shape.GetPtr())));
            }

            shape = shapeCache[const_cast<JPH::Shape *>(shape.GetPtr())][scale];
        }

        compoundShapeSettings.AddShape(ToJPH(loc), ToJPHRotation(rot), shape);

        if (!mShapeChangesEventsPerObjectForComponents[gameObject->GetObjectUUID()].Contains(collider->Uuid)) {
            mShapeChangesEventsPerObjectForComponents[gameObject->GetObjectUUID()][collider->Uuid] = collider->SubscribeToEvent("ShapeChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            collider->SubscribeToEvent("RelativeLocationChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            collider->SubscribeToEvent("RelativeRotationChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            collider->SubscribeToEvent("RelativeScaleChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
        }
    }

#ifdef PLU_ENGINE_EDITOR_BUILD
    for (auto mesh : mStaticMeshesUsageInObjects) {
        if (mesh.second.Contains(uuid)) {
            mesh.second.Remove(uuid);
        }
    }
#endif

    for (auto staticMeshCollider : staticMeshColliders) {
        TUsePointer<StaticMeshComponent> staticMeshComponent = staticMeshCollider;
        TUsePointer<StaticMesh> staticMesh = staticMeshComponent->GetStaticMesh();

        if (!mShapeChangesEventsPerObjectForComponents[gameObject->GetObjectUUID()].Contains(staticMeshComponent->Uuid)) {
            mShapeChangesEventsPerObjectForComponents[gameObject->GetObjectUUID()][staticMeshComponent->Uuid] = staticMeshComponent->SubscribeToEvent("StaticMeshChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            staticMeshComponent->SubscribeToEvent("RelativeLocationChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            staticMeshComponent->SubscribeToEvent("RelativeRotationChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
            staticMeshComponent->SubscribeToEvent("RelativeScaleChanged", [this, gameObject](void*) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            });
        }

        if (!staticMesh) continue;

        Matrix4 worldMatrix = staticMeshComponent->GetMatrixRelativeToGameObject();
        Vec3 loc = GetLocationFromMatrix(worldMatrix);
        Vec3 rot = GetRotationFromMatrix(worldMatrix);
        Vec3 scale = staticMeshComponent->GetWorldScale();

        if (staticMesh->CollisionName != "") {
            if (!staticMesh->CollisionData || (staticMesh->CollisionName != staticMesh->CollisionData->GetClass()->TypeName)) {
                TypeInfo* newData = TypeRegistry::GetInstance()->GetTypeOfName(staticMesh->CollisionName);
                if (newData->IsDerivedOfOrSame(IStaticMeshCollisionData::GetStaticClass())) {
                    staticMesh->CollisionData = TOwningPointer(static_cast<IStaticMeshCollisionData*>(newData->Construct()));
                }
            }
            JPH::ShapeRefC shape = staticMesh->CollisionData->GetShape(staticMesh.GetRaw());

            if (scale != Vec3(1.0f)) {
                shape = new JPH::ScaledShape(shape.GetPtr(), ToJPH(scale));
            }
            compoundShapeSettings.AddShape(ToJPH(loc + staticMesh->CollisionData->GetOffset(staticMesh.GetRaw(), scale)), ToJPHRotation(rot), shape);
        }

#ifdef PLU_ENGINE_EDITOR_BUILD
        mStaticMeshesUsageInObjects[staticMesh->Uuid].Insert(staticMeshCollider->GetParentGameObject()->GetObjectUUID());
#endif
    }

    if (compoundShapeSettings.mSubShapes.empty()) {
        if (mBodyPerObject.Contains(gameObject->GetObjectUUID())) {
            mBodyPerObject.Remove(gameObject->GetObjectUUID());
        }
        return;
    }

    JPH::Shape::ShapeResult result = compoundShapeSettings.Create();
    if (result.HasError()) {
        PLU_CORE_ERROR("Failed to create shape for collider, error {}", result.GetError());
        return;
    }
    JPH::ShapeRefC finalShape = result.Get();

    if (mBodyPerObject.Contains(gameObject->GetObjectUUID())) {
        mBodyPerObject.Remove(gameObject->GetObjectUUID());
    }

    TOwningPointer<PhysicsBody> body = CreateOwning<PhysicsBody>(
        this->mPhysicsSystem->GetBodyInterface(),
        finalShape,
        ToJPH(gameObject->GetObjectLocation()),
        ToJPHRotation(gameObject->GetObjectRotation()),
        bodyComponent->Type,
        bodyComponent->Friction,
        bodyComponent->Restitution,
        bodyComponent->Mass
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
        Int32 scaleEvent = gameObject->SubscribeToEvent("ScaleChange", [this, gameObject](void*) {
            if (mBodyPerObject.Contains(gameObject->GetObjectUUID()) && !mIsUpdatingObjectsFromPhysics) {
                RebuildObjectCollision(gameObject->GetObjectUUID());
            }
        });
        mRotLocChangesEventsPerObject.Insert(gameObject->GetObjectUUID(), {locEvent, rotEvent});
    }
}

#ifdef PLU_ENGINE_EDITOR_BUILD
void Plu::PhysicsWorld::RebuildObjectsThatUseMesh(StaticMesh *staticMesh)
{
    for (auto object : mStaticMeshesUsageInObjects[staticMesh->Uuid]) {
        RebuildObjectCollision(object);
    }
}
#endif

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

    mPointRenderer = CreateOwning<JoltPointRenderer>();
    mWireframeRenderer = CreateOwning<JoltWireframeRenderer>();

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
        this->OnUpdate(deltaTime, true);
    });

    sceneWorld->SubscribeToEvent("NewComponent", [this](void* data) {
        TUsePointer<GameObjectComponent> newComponent = *static_cast<TUsePointer<GameObjectComponent>*>(data);
        TUsePointer<GameObject> parentObject = newComponent->GetParentGameObject();
        if (newComponent->GetClass()->IsDerivedOfOrSame(PhysicsColliderComponent::GetStaticClass()) ||
        newComponent->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())
        ) {
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

        if (newComponent->GetClass() == PhysicsBodyComponent::GetStaticClass()) {
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
        if (oldComponent->GetClass()->IsDerivedOfOrSame(PhysicsColliderComponent::GetStaticClass()) ||
        oldComponent->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())
        )
        {

            mShapeChangesEventsPerObjectForComponents[parentObject->GetObjectUUID()].Remove(oldComponent->Uuid);

            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

        if (oldComponent->GetClass() == PhysicsBodyComponent::GetStaticClass()) {
            mObjectsToCheck.Insert(parentObject->GetObjectUUID());
        }

    });
}

void Plu::PhysicsWorld::OnUpdate(float deltaTime, bool updateBodies)
{
    if (!mObjectsToCheck.IsEmpty()) {
        for (auto uuid : mObjectsToCheck) {
            RebuildObjectCollision(uuid);
        }
        mObjectsToCheck.Clear();
    }
    PLU_PROFILE_SCOPE("Physics Tick");
    if (updateBodies) mPhysicsSystem->Update(deltaTime, 1, mAllocator.GetRaw(), JoltPhysics::GetJoltThreadPool().GetRaw());

    mIsUpdatingObjectsFromPhysics = true;
    TUsePointer<SceneWorld> sceneWorld = mApplicationInfo->AppObjectManager->GetObjectAsUser<SceneWorld>(mSceneWorldHandle);
    DynamicArray<UInt64> toDestroy;
    for (const auto& body : mBodyPerObject) {
        TUsePointer<PhysicsBody> actualBody = body.second;
        TUsePointer<GameObject> gameObject = sceneWorld->GetGameObjectByUUID(body.first);

        if (!gameObject) {
            toDestroy.PushBack(body.first);
            continue;
        }

        gameObject->SetObjectLocation(ToGLM(actualBody->GetPosition()));

        JPH::Quat jphRot = actualBody->GetRotation();
        glm::quat glmRot(jphRot.GetW(), jphRot.GetX(), jphRot.GetY(), jphRot.GetZ());
        Vec3 eulerDeg = glm::degrees(glm::eulerAngles(glmRot));
        gameObject->SetObjectRotation(eulerDeg);
    }
    mIsUpdatingObjectsFromPhysics = false;
    for (const auto& destroy : toDestroy) {
        RebuildObjectCollision(destroy);
    }

    if (DebugRenderMode == PhysicsDebugRenderMode::NONE) return;

    mPointRenderer->BeginFrame();
    mWireframeRenderer->BeginFrame();

    JPH::BodyIDVector bodies;
    mPhysicsSystem->GetBodies(bodies);
    for (JPH::BodyID body : bodies)
    {
        JPH::BodyLockRead lock(mPhysicsSystem->GetBodyLockInterface(), body);
        if (!lock.Succeeded()) continue;
        if (DebugRenderMode == PhysicsDebugRenderMode::WIREFRAME) mWireframeRenderer->AddBody(lock.GetBody(), DebugLineColor);
        if (DebugRenderMode == PhysicsDebugRenderMode::POINTS) mPointRenderer->AddBody(lock.GetBody(), DebugPointColor);
    }

    mWireframeRenderer->PackInto(sceneWorld->GetRawDebugLineArray());
    mPointRenderer->PackInto(sceneWorld->GetRawDebugPointArray());
}

unsigned int Plu::PhysicsWorld::GetNumOfBodies() const
{
    return mPhysicsSystem->GetNumBodies();
}
