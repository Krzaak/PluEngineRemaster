//
// Created by Plutex on 2026-02-14.
//

#include "PluEngine/GameCore/GameMode.h"

#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"

Plu::GameMode::GameMode()
{
}

void Plu::GameMode::OnSetupComponents()
{
	ControllerClass = Controller::GetStaticClass();
	PuppetClass = SpectatorPuppet::GetStaticClass();
}
