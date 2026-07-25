//
// Created by Plutex on 4/6/26.
//

#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include "PluEngine/PluUtils.h"
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"

using namespace Plu;

namespace
{
    // A component's placement inside the body, decomposed for Jolt. The body sits at the game
    // object's location/rotation, so body space still carries the object's scale plus the whole
    // chain of component-relative transforms — hence GetMatrixRelativeToGameObject() rather than
    // the component's own relative transform, which is only correct for directly attached ones.
    struct SubShapePlacement
    {
        Vec3 Location;
        Vec3 RotationDegrees;
        Vec3 Scale;
    };

    SubShapePlacement MakeSubShapePlacement(WorldComponent* component, Vec3 objectScale)
    {
        Matrix4 matrix = glm::scale(glm::mat4(1.0f), objectScale) * component->GetMatrixRelativeToGameObject();
        return {
            GetLocationFromMatrix(matrix),
            GetRotationFromMatrix(matrix),
            GetScaleFromMatrix(matrix)
        };
    }

    JPH::Quat ToJPHRotation(Vec3 rotationDegrees)
    {
        return JPH::Quat::sEulerAngles(JPH::Vec3(
            JPH::DegreesToRadians(rotationDegrees.x),
            JPH::DegreesToRadians(rotationDegrees.y),
            JPH::DegreesToRadians(rotationDegrees.z)
        ));
    }
}

Plu::PhysicsCompoundShape::PhysicsCompoundShape()
    : mCompoundShape(nullptr)
{
}

void PhysicsCompoundShape::Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies,
                                DynamicArray<TUsePointer<StaticMeshComponent>> meshComponents,
                                Vec3 parentScale,
                                MeshCollisionShapeCache* shapeCache)
{
    JPH::StaticCompoundShapeSettings compoundSettings;
    bool hasValidShape = false;

    for (const auto& bodyPtr : bodies)
    {
        if (!bodyPtr) continue;

        JPH::ShapeRefC shape = bodyPtr->GetShape();
        if (shape == nullptr)
        {
            PLU_CORE_ERROR("CompoundShapeWrapper::Init - Found null shape in PhysicsBodyComponent. Skipping entry.");
            continue;
        }

        SubShapePlacement placement = MakeSubShapePlacement(bodyPtr.GetRaw(), parentScale);

        if (glm::any(glm::notEqual(placement.Scale, Vec3(1.0f))))
        {
            shape = new JPH::ScaledShape(shape.GetPtr(), ToJPH(placement.Scale));
        }

        compoundSettings.AddShape(ToJPH(placement.Location), ToJPHRotation(placement.RotationDegrees), shape);
        hasValidShape = true;
    }

    for (const auto& smComp : meshComponents)
    {
        if (!smComp || !smComp->StaticMeshToDisplay) continue;
        StaticMesh* mesh = smComp->StaticMeshToDisplay.GetRaw();
        if (!mesh || mesh->CollisionShapes.IsEmpty()) continue;

        SubShapePlacement placement = MakeSubShapePlacement(smComp.GetRaw(), parentScale);
        JPH::Quat jphRotation = ToJPHRotation(placement.RotationDegrees);

        // The scale is baked into the shapes themselves here (no ScaledShape wrapper), so LocalOffset
        // already comes back scaled — it still has to be rotated into the component's orientation
        // before being added to the component's position.
        Quaternion shapeRotation = Quaternion(glm::radians(placement.RotationDegrees));

        DynamicArray<MeshCollisionShapeEntry> shapeEntries = shapeCache
            ? GetOrBuildCollisionShapesForMesh(*shapeCache, mesh, placement.Scale)
            : BuildCollisionShapesForMesh(mesh, placement.Scale);
        for (const auto& entry : shapeEntries)
        {
            JPH::Vec3 shapePos = ToJPH(placement.Location + shapeRotation * entry.LocalOffset);
            compoundSettings.AddShape(shapePos, jphRotation, entry.Shape);
            hasValidShape = true;
        }
    }

    if (!hasValidShape)
    {
        PLU_CORE_ERROR("CompoundShapeWrapper::Init - No valid shapes were added to the compound settings.");
        return;
    }

    JPH::Shape::ShapeResult result = compoundSettings.Create();
    if (result.HasError())
    {
        PLU_CORE_ERROR("CompoundShapeWrapper::Init - Jolt Shape Creation Error: " + std::string(result.GetError().c_str()));
        return;
    }

    mCompoundShape = result.Get();
}
