//
// Created by Plutex on 2026-02-19.
//

#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"

#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/GameCore/Controller.h"

void Plu::SpectatorPuppet::OnSetupComponents()
{
	Camera = AddComponent(CameraComponent::GetStaticClass(), "SpectatorCamera");

	GetInputHandler()->AddActionOnHold(Key::W, [this](){mDirection += Vec3(0,0,-1);});
	GetInputHandler()->AddActionOnHold(Key::S, [this](){mDirection += Vec3(0,0,1);});
	GetInputHandler()->AddActionOnHold(Key::A, [this](){mDirection += Vec3(-1,0,0);});
	GetInputHandler()->AddActionOnHold(Key::D, [this](){mDirection += Vec3(1,0,0);});
	GetInputHandler()->AddActionOnHold(Key::C, [this](){mDirection += Vec3(0,-1,0);});
	GetInputHandler()->AddActionOnHold(Key::Space, [this](){mDirection += Vec3(0,1,0);});
}

void Plu::SpectatorPuppet::OnUpdate(float deltaTime)
{
	if (mDirection == Vec3(0,0,0)) return;
	mDirection = glm::normalize(mDirection);
	mDirection *= MovementSpeed;
	SetObjectLocation(GetObjectLocation() + mDirection);
	mDirection = Vec3(0);
}

void Plu::SpectatorPuppet::OnBeginPlay()
{
	GetController()->HideCursor();
}
