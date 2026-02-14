//
// Created by Plutex on 2026-02-14.
//

#include "PluEngine/GameCore/Controller.h"

#include "PluEngine/GameCore/Puppet.h"

void Plu::Controller::Possess(TUsePointer<Puppet> puppet)
{
	PLU_ASSERT(puppet, "Puppet is invalid on Possess!")
	mPossessedPuppet = puppet;
	puppet->OnPossessed(This());
}

void Plu::Controller::Unpossess()
{
	if (!mPossessedPuppet) return;
	mPossessedPuppet->OnUnpossessed();
	mPossessedPuppet = nullptr;
}

Plu::TUsePointer<Plu::Puppet> Plu::Controller::GetControllerPuppet()
{
	return mPossessedPuppet;
}
