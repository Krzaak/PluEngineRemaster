//
// Created by Plutex on 2026-02-18.
//

#include "PluEngine/Gameplay/InputManager.h"

#include "PluEngine/Platform/InputBackendFactory.h"

Plu::InputManager::InputManager()
{
	mInputBackend = InputBackendFactory::Create();
}

Plu::InputManager::~InputManager()
{
}

bool Plu::InputManager::IsKeyDown(Key key) const {
	return mInputBackend->GetKeyboard().IsDown(key);
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
