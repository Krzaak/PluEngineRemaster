//
// Created by Plutex on 4/6/26.
//

#ifndef PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
#define PLUENGINE_PHYSICSCOMPOUNDSHAPE_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/GameObject/GameObjectComponent.h"

#include "PhysicsCompoundShape.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PhysicsCompoundShapeComponent : public GameObjectComponent
    {
        REFLECTION_BODY_PHYSICSCOMPOUNDSHAPECOMPONENT()
    public:
        PhysicsCompoundShapeComponent();
        virtual ~PhysicsCompoundShapeComponent() = default;

        void Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies);

        JPH::ShapeRefC GetCompoundShape() const { return mCompoundShape; }

    private:
        JPH::ShapeRefC mCompoundShape;
    };
}

#endif //PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
