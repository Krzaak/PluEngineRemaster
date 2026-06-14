//
// Created by Plutex on 4/6/26.
//

#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include "PluEngine/PluUtils.h"
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"

using namespace Plu;

Plu::PhysicsCompoundShape::PhysicsCompoundShape()
    : mCompoundShape(nullptr)
{
}

void PhysicsCompoundShape::Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies,
                                DynamicArray<TUsePointer<StaticMeshComponent>> meshComponents,
                                Vec3 parentScale)
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

        Vec3 combinedScale = parentScale * bodyPtr->GetRelativeScale();
        if (glm::any(glm::notEqual(combinedScale, Vec3(1.0f))))
        {
            shape = new JPH::ScaledShape(shape.GetPtr(), JPH::Vec3(combinedScale.x, combinedScale.y, combinedScale.z));
        }

        Vec3 rotEulerDeg = bodyPtr->GetRelativeRotation();
        JPH::Quat jphRotation = JPH::Quat::sEulerAngles(
            JPH::Vec3(
                JPH::DegreesToRadians(rotEulerDeg.x),
                JPH::DegreesToRadians(rotEulerDeg.y),
                JPH::DegreesToRadians(rotEulerDeg.z)
            )
        );

        JPH::Vec3 jphPosition = ToJPH(bodyPtr->GetRelativeLocation() * parentScale);

        compoundSettings.AddShape(jphPosition, jphRotation, shape);
        hasValidShape = true;
    }

    for (const auto& smComp : meshComponents)
    {
        if (!smComp || !smComp->StaticMeshToDisplay) continue;
        StaticMesh* mesh = smComp->StaticMeshToDisplay.GetRaw();
        if (!mesh || mesh->CollisionShapes.IsEmpty()) continue;

        Vec3 combinedScale = parentScale * smComp->GetRelativeScale();

        Vec3 rotEulerDeg = smComp->GetRelativeRotation();
        JPH::Quat jphRotation = JPH::Quat::sEulerAngles(
            JPH::Vec3(
                JPH::DegreesToRadians(rotEulerDeg.x),
                JPH::DegreesToRadians(rotEulerDeg.y),
                JPH::DegreesToRadians(rotEulerDeg.z)
            )
        );
        JPH::Vec3 jphPosition = ToJPH(smComp->GetRelativeLocation() * parentScale);

        DynamicArray<MeshCollisionShapeEntry> shapeEntries = BuildCollisionShapesForMesh(mesh, combinedScale);
        for (const auto& entry : shapeEntries)
        {
            JPH::Vec3 shapePos = jphPosition + JPH::Vec3(entry.LocalOffset.x, entry.LocalOffset.y, entry.LocalOffset.z);
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
