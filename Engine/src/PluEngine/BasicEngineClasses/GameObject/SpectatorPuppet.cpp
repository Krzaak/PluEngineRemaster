//
// Created by Plutex on 2026-02-19.
//

#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"

#include "PluEngine/GameCore/Controller.h"

void Plu::SpectatorPuppet::OnUpdate(float deltaTime)
{
	if (GetController()->IsKeyboardKeyDown(Key::W, This())) {
		SetObjectLocation(GetObjectLocation() + Vec3(1,0,0));
	}
}
