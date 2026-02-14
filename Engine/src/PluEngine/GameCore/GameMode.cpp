//
// Created by Plutex on 2026-02-14.
//

#include "PluEngine/GameCore/GameMode.h"

Plu::GameMode::GameMode()
{
	ControllerClass = Controller::GetStaticClass();
	PuppetClass = Puppet::GetStaticClass();
}
