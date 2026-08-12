//
// Created by Plutex on 2026-02-19.
//

#include "PluEngine/Gameplay/Objects/SpectatorPuppet.h"

#include "PluEngine/PluUtils.h"
#include "PluEngine/Gameplay/Components/CameraComponent.h"
#include "PluEngine/Gameplay/Controller.h"

void Plu::SpectatorPuppet::OnSetupComponents()
{
	Camera = AddComponent(CameraComponent::GetStaticClass(), "SpectatorCamera");

	GetInputHandler()->AddActionOnHold(Key::W, [this](){mDirection += GetObjectForwardVector();});
	GetInputHandler()->AddActionOnHold(Key::S, [this](){mDirection -= GetObjectForwardVector();});
	GetInputHandler()->AddActionOnHold(Key::A, [this](){mDirection -= GetObjectRightVector();});
	GetInputHandler()->AddActionOnHold(Key::D, [this](){mDirection += GetObjectRightVector();});
	GetInputHandler()->AddActionOnHold(Key::C, [this](){mDirection += Vec3(0,-1,0);});
	GetInputHandler()->AddActionOnHold(Key::Space, [this](){mDirection += Vec3(0,1,0);});
	GetInputHandler()->AddActionOnHold(Key::LeftShift, [this](){mSprinting = true;});
}

void Plu::SpectatorPuppet::OnUpdate(float deltaTime)
{
	float pitch = GetInputHandler()->GetMouseDeltaY() * MouseSensitivity;
	pitch = ClampAngle(pitch + GetController()->GetControlRotation().x, -89.9, 89.9);
	Vec3 newRot = Vec3(pitch, GetInputHandler()->GetMouseDeltaX() * -1 * MouseSensitivity + GetController()->GetControlRotation().y, 0);
	GetController()->SetControlRotation(newRot);
	SetObjectRotation(GetController()->GetControlRotationForPuppet());
	if (mDirection != Vec3(0, 0, 0))
	{
		float speed = MovementSpeed * (mSprinting ? SprintSpeedMultiplier : 1.f);
		mDirection = glm::normalize(mDirection);
		mDirection *= speed * deltaTime;
		SetObjectLocation(GetObjectLocation() + mDirection);
		mDirection = Vec3(0);
	}
	mSprinting = false;
}

void Plu::SpectatorPuppet::OnBeginPlay()
{
	GetController()->HideCursor();
}
