//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBoxComponent.h"

#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/PluUtils.h"

Plu::PhysicsBoxComponent::PhysicsBoxComponent()
{
	BoxSize = {1,1,1};
}

Plu::PhysicsBoxComponent::~PhysicsBoxComponent()
{
}

JPH::ShapeRefC Plu::PhysicsBoxComponent::GetShape()
{
	float x = Plu::ClampF(BoxSize.x, 0.001f, FLT_MAX);
	float y = Plu::ClampF(BoxSize.y, 0.001f, FLT_MAX);
	float z = Plu::ClampF(BoxSize.z, 0.001f, FLT_MAX);
	JPH::ShapeRefC shape = new JPH::BoxShape(JPH::Vec3(x, y, z));
	return shape;
}
