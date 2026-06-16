//
// Created by Plutex on 2026-02-28.
//

#include "PluEngine/GameCore/Puppet.h"

Plu::TUsePointer<Plu::Controller> Plu::Puppet::GetController()
{
	return mController;
}

Plu::InputHandler *Plu::Puppet::GetInputHandler()
{
	return &mPuppetInputHandler;
}
