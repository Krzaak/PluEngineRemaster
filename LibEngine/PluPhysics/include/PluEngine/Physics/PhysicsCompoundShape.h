//
// Created by Plutex on 4/6/26.
//

#ifndef PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
#define PLUENGINE_PHYSICSCOMPOUNDSHAPE_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include "PluEngine/Gameplay/Components/PhysicsBodyComponent.h"
#include "PluEngine/Gameplay/Components/StaticMeshComponent.h"
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"

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

        // shapeCache (optional) memoises the per-mesh collision geometry across calls. Pass one when
        // building many bodies in a row — a scene load hits the same mesh assets over and over, and
        // rebuilding a ConvexHull/MeshShape per object dominates the cost otherwise.
        void Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies,
                  DynamicArray<TUsePointer<StaticMeshComponent>> meshComponents,
                  Vec3 parentScale = Vec3(1.0f),
                  MeshCollisionShapeCache* shapeCache = nullptr);

        JPH::ShapeRefC GetCompoundShape() const { return mCompoundShape; }

    private:
        JPH::ShapeRefC mCompoundShape;
    };
}

#endif //PLUENGINE_PHYSICSCOMPOUNDSHAPE_H
