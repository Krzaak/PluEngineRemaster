//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Gameplay/Components/PhysicsBoxComponent.h"

#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PluEngine/Gameplay/GameObject.h"
#include "PluEngine/Gameplay/Scenes/ScenesManager.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"
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
	JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(JPH::Vec3(x, y, z));
	shape->SetUserData(MakeMaterialUserData()); // per-sub-shape friction/restitution/channel
	return JPH::ShapeRefC(shape.GetPtr());
}
