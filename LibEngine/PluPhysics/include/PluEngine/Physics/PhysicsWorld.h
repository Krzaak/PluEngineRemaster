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
    class ObjectLayerPairFilterImpl;
    class ObjectVsBroadPhaseLayerFilterImpl;
    class BPLayerInterfaceImpl;


    PLU_CLASS()
    class PLUPHYSICS_API PhysicsWorld : public EngineObject
    {
        REFLECTION_BODY_PHYSICSWORLD()
    private:
        friend void JoltPhysics::Init(ApplicationInfo* applicationInfo);

        EngineObjectHandle mSceneWorldHandle;
        ApplicationInfo* mApplicationInfo;

        TOwningPointer<JPH::TempAllocatorImpl>                 mAllocator;
        TOwningPointer<JPH::PhysicsSystem>                     mPhysicsSystem;
        TOwningPointer<BPLayerInterfaceImpl>                   mBPLayerInterface;
        TOwningPointer<ObjectVsBroadPhaseLayerFilterImpl>      mObjVsBPFilter;
        TOwningPointer<ObjectLayerPairFilterImpl>              mObjVsObjFilter;
    public:
        PhysicsWorld();
        virtual ~PhysicsWorld() override;

        void Init();
        void OnUpdate(float deltaTime);
    };
}

#endif //PLUENGINE_PHYSICSWORLD_H
