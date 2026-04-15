//
// Created by Plutex on 4/6/26.
//

// CompoundShapeWrapper.cpp
#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include "PluEngine/PluUtils.h"

using namespace Plu;

Plu::PhysicsCompoundShape::PhysicsCompoundShape()
    : mCompoundShape(nullptr)
{
}

void PhysicsCompoundShape::Init(DynamicArray<TUsePointer<PhysicsBodyComponent>> bodies)
{
    if (bodies.IsEmpty())
    {
        PLU_CORE_ERROR("CompoundShapeWrapper::Init - Received empty bodies array. Aborting.");
        return;
    }

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

        // Convert GLM Vec3 Euler (degrees) to JPH Quat
        Vec3 rotEulerDeg = bodyPtr->GetRelativeRotation();
        JPH::Quat jphRotation = JPH::Quat::sEulerAngles(
            JPH::Vec3(
                JPH::DegreesToRadians(rotEulerDeg.x),
                JPH::DegreesToRadians(rotEulerDeg.y),
                JPH::DegreesToRadians(rotEulerDeg.z)
            )
        );

        JPH::Vec3 jphPosition = ToJPH(bodyPtr->GetRelativeLocation());

        compoundSettings.AddShape(jphPosition, jphRotation, shape);
        hasValidShape = true;
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
