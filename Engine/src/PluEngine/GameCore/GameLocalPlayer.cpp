//
// Created by Plutex on 2026-02-12.
//

#include "PluEngine/GameCore/GameLocalPlayer.h"

#include "PluEngine/GameCore/Controller.h"
#include "PluEngine/Managers/ScenesManager.h"

void Plu::GameLocalPlayer::Init(const TUsePointer<IScenesManager> &sceneManager, UInt16 id)
{
	mScenesManager = sceneManager;
	mLocalPlayerIndex = id;
}

void Plu::GameLocalPlayer::OnKeyboardKeyUpdate(Key key, ButtonState state)
{
	TUsePointer<Controller> controller = mScenesManager->GetCurrentWorld()->GetControllerByID(mLocalPlayerIndex);
	InputHandler* controllerInputHandler = controller->GetInputHandler();
	controllerInputHandler->UpdateKeyState(key, state);
	TUsePointer<Puppet> puppet = controller->GetControllerPuppet();
	if (!puppet) return;
	puppet->GetInputHandler()->UpdateKeyState(key, state);
}
