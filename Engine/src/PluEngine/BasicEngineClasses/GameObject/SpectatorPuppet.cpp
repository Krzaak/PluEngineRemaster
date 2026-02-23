//
// Created by Plutex on 2026-02-19.
//

#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"

#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/GameCore/Controller.h"

void Plu::SpectatorPuppet::OnSetupComponents()
{
	Camera = AddComponent(CameraComponent::GetStaticClass());
}

void Plu::SpectatorPuppet::OnUpdate(float deltaTime)
{
	if (GetController()->IsKeyboardKeyDown(Key::W, This())) {
		SetObjectLocation(GetObjectLocation() + Vec3(0,0,-1));
	}
	if (GetController()->IsKeyboardKeyDown(Key::S, This())) {
		SetObjectLocation(GetObjectLocation() + Vec3(0,0,1));
	}
}
