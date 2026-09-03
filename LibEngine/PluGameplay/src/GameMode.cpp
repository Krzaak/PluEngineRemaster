//
// Created by Plutex on 2026-02-14.
//

#include "PluEngine/Gameplay/GameMode.h"

#include "PluEngine/Gameplay/Objects/SpectatorPuppet.h"

Plu::GameMode::GameMode()
{
}

void Plu::GameMode::OnSetupComponents()
{
	ControllerClass = Controller::GetStaticClass();
	PuppetClass = SpectatorPuppet::GetStaticClass();
}
