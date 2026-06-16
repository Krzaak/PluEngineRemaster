//
// Created by Plutex on 2026-02-18.
//

#include "PluEngine/Input/InputManager.h"

#include "PluEngine/Input/InputBackendFactory.h"

Plu::InputManager::InputManager()
{
	mInputBackend = InputBackendFactory::Create();
}

Plu::InputManager::~InputManager()
{
}

void Plu::InputManager::Init(const TUsePointer<GameClient> &gameClient, TUsePointer<IWindow> &window)
{
	mInputBackend->mGameClient = gameClient;
	mInputBackend->mWindow = window;
}

Plu::TUsePointer<PlatformInputBackend> Plu::InputManager::GetInputBackend()
{
	return mInputBackend;
}
