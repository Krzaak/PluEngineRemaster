//
// Created by Plutex on 2026-02-11.
//
#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"

void Plu::CameraComponent::OnUpdate(float deltaTime)
{
	WorldComponent::OnUpdate(deltaTime);
}

void Plu::CameraComponent::OnBeginPlay()
{
	WorldComponent::OnBeginPlay();
}

void Plu::CameraComponent::OnEndPlay()
{
	WorldComponent::OnEndPlay();
}

Vec3 Plu::CameraComponent::GetCameraLocation()
{
	return GetWorldLocation();
}

Plu::CameraOptions * Plu::CameraComponent::GetCameraOptions()
{
	return &Options;
}

Vec3 Plu::CameraComponent::GetCameraRotation()
{
	return GetWorldRotation();
}
