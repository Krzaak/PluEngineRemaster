//
// Created by Plutex on 4/6/26.
//

#ifndef PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
#define PLUENGINE_PHYSICSCOMPOUNDSHAPE_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"

#include "PhysicsCompoundShape.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PhysicsCompoundShape : public EngineObject
    {
        REFLECTION_BODY_PHYSICSCOMPOUNDSHAPE()
    public:
        PhysicsCompoundShape();
        virtual ~PhysicsCompoundShape() = default;

        void Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies,
                  DynamicArray<TUsePointer<StaticMeshComponent>> meshComponents,
                  Vec3 parentScale = Vec3(1.0f));

        JPH::ShapeRefC GetCompoundShape() const { return mCompoundShape; }

    private:
        JPH::ShapeRefC mCompoundShape;
    };
}

#endif //PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
