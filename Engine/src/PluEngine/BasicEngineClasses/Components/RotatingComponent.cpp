//
// Created by Plutex on 2026-02-11.
//

#include "PluEngine/BasicEngineClasses/Components/RotatingComponent.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/GameObject/GameObject.h"

void Plu::RotatingComponent::OnBeginPlay()
{
	GameObjectComponent::OnBeginPlay();
}

void Plu::RotatingComponent::OnEndPlay()
{
	GameObjectComponent::OnEndPlay();
}

void Plu::RotatingComponent::OnUpdate(float deltaTime)
{
	if (!GetParentGameObject()) return;
	if (RotateAroundYaw) {
		Vec3 rot = GetParentGameObject()->GetObjectRotation();
		rot.y += RotateYawSpeed;
		GetParentGameObject()->SetObjectRotation(rot);
	}
	if (RotateAroundPitch) {
		Vec3 rot = GetParentGameObject()->GetObjectRotation();
		rot.z += RotatePitchSpeed;
		GetParentGameObject()->SetObjectRotation(rot);
	}
	if (RotateAroundRoll) {
		Vec3 rot = GetParentGameObject()->GetObjectRotation();
		rot.x += RotateRollSpeed;
		GetParentGameObject()->SetObjectRotation(rot);
	}
}
