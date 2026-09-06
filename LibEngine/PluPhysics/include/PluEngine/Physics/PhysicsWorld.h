//
// Created by Plutex on 9/5/26.
//

#ifndef PLUENGINE_PHYSICSWORLD_H
#define PLUENGINE_PHYSICSWORLD_H

#include "PluEngine/Core.h"
#include "JoltIntializer.h"
#include "PluEngine/Core/Objects/EngineObject.h"
#include "PhysicsWorld.generated.h"

namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
}

namespace Plu
{
    class JoltPointRenderer;
    class JoltWireframeRenderer;
    class PhysicsBody;
    class PhysicsBodyComponent;
    class PhysicsColliderComponent;
    class ObjectLayerPairFilterImpl;
    class ObjectVsBroadPhaseLayerFilterImpl;
    class BPLayerInterfaceImpl;

    PLU_ENUM(PyNamespace=Plu)
    enum class PhysicsDebugRenderMode
    {
        NONE,
        POINTS,
        WIREFRAME
    };


    PLU_CLASS()
    class PLUPHYSICS_API PhysicsWorld : public EngineObject
    {
        REFLECTION_BODY_PHYSICSWORLD()
    private:
        friend void JoltPhysics::Init(ApplicationInfo* applicationInfo);

        EngineObjectHandle mSceneWorldHandle;
        ApplicationInfo* mApplicationInfo;

        //My own associations
        GameHashMap<UInt64, DynamicArray<TUsePointer<PhysicsColliderComponent>>> mCollidersPerObject;
        GameHashMap<UInt64, TUsePointer<PhysicsBodyComponent>> mBodyComponentPerObject;

        HashSet<UInt64> mObjectsToCheck;

        GameHashMap<UInt64, TOwningPointer<PhysicsBody>> mBodyPerObject;
        GameHashMap<UInt64, std::pair<Int32, Int32>> mRotLocChangesEventsPerObject;
        GameHashMap<UInt64, GameHashMap<UInt64, Int32>> mShapeChangesEventsPerObjectForComponents;

        bool mIsUpdatingObjectsFromPhysics = false;

        //Jolt stuff
        TOwningPointer<JPH::TempAllocatorImpl>                 mAllocator;
        TOwningPointer<JPH::PhysicsSystem>                     mPhysicsSystem;
        TOwningPointer<BPLayerInterfaceImpl>                   mBPLayerInterface;
        TOwningPointer<ObjectVsBroadPhaseLayerFilterImpl>      mObjVsBPFilter;
        TOwningPointer<ObjectLayerPairFilterImpl>              mObjVsObjFilter;

        void RebuildObjectCollision(UInt64 uuid);

        TOwningPointer<JoltWireframeRenderer> mWireframeRenderer;
        TOwningPointer<JoltPointRenderer> mPointRenderer;
    public:
        PhysicsWorld();
        virtual ~PhysicsWorld() override;

        void Init();
        void OnUpdate(float deltaTime);

        unsigned int GetNumOfBodies() const;

        PhysicsDebugRenderMode DebugRenderMode = PhysicsDebugRenderMode::WIREFRAME;
        Vec3 DebugLineColor = Vec3(1.0f, 0.0f, 0.0f);
        Vec3 DebugPointColor = Vec3(1.0f, 0.0f, 0.0f);
    };
}

#endif //PLUENGINE_PHYSICSWORLD_H
