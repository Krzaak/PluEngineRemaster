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
	//
	// Each one re-checks the client and the player. The captured TUsePointer lives as long as the
	// backend does — the whole session — while the client only lives as long as the game, so an
	// event arriving after EndGame() would otherwise dereference a dead object. ClearInputSink()
	// below unhooks them at that point; this is the safety net for anything already in flight.
	auto localPlayer = [gameClient]() -> TUsePointer<GameLocalPlayer> {
		if (!gameClient) return nullptr;
		return gameClient->GetLocalPlayerByID(0);
	};
	mInputBackend->mInputSink.OnKeyboardKey = [localPlayer](Key key, ButtonState state) {
		if (TUsePointer<GameLocalPlayer> player = localPlayer()) player->OnKeyboardKeyUpdate(key, state);
	};
	mInputBackend->mInputSink.OnMouseKey = [localPlayer](MouseButton button, ButtonState state) {
		if (TUsePointer<GameLocalPlayer> player = localPlayer()) player->OnMouseKeyUpdate(button, state);
	};
	mInputBackend->mInputSink.OnMouse = [localPlayer](MouseState& state) {
		if (TUsePointer<GameLocalPlayer> player = localPlayer()) player->OnMouseUpdate(state);
	};
	mInputBackend->mWindow = window;
}

void Plu::InputManager::ClearInputSink()
{
	// Called when the game ends: the callbacks captured the client that is going away, so they are
	// unhooked rather than left to fire at a dead object.
	if (mInputBackend) mInputBackend->mInputSink = InputSink{};
}

Plu::TUsePointer<PlatformInputBackend> Plu::InputManager::GetInputBackend()
{
	return mInputBackend;
}
