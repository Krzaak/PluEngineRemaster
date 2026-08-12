//
// Created by Plutex on 2026-02-18.
//

#include "PluEngine/Gameplay/InputManager.h"
#include "PluEngine/Gameplay/GameClient.h"
#include "PluEngine/Gameplay/GameLocalPlayer.h"

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
	// Route the backend's notifications at the active local player. The backend lives in the
	// platform layer and cannot name GameClient, so gameplay hands it plain callbacks.
	mInputBackend->mInputSink.OnKeyboardKey = [gameClient](Key key, ButtonState state) {
		gameClient->GetLocalPlayerByID(0)->OnKeyboardKeyUpdate(key, state);
	};
	mInputBackend->mInputSink.OnMouseKey = [gameClient](MouseButton button, ButtonState state) {
		gameClient->GetLocalPlayerByID(0)->OnMouseKeyUpdate(button, state);
	};
	mInputBackend->mInputSink.OnMouse = [gameClient](MouseState& state) {
		gameClient->GetLocalPlayerByID(0)->OnMouseUpdate(state);
	};
	mInputBackend->mWindow = window;
}

Plu::TUsePointer<PlatformInputBackend> Plu::InputManager::GetInputBackend()
{
	return mInputBackend;
}
