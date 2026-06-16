//
// Created by Plutex on 2026-02-12.
//

#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/GameCore/GameLocalPlayer.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/GameCore/GameLocalPlayer.h"

Plu::GameClient::GameClient(const TUsePointer<EngineObjectManager> &objectManager,
                            const TUsePointer<SceneManager> &scenesManager, const TUsePointer<InputManager> &inputManager, TUsePointer<IWindow> window)
{
	mObjectManager = objectManager;
	mInputManager = inputManager;
	mScenesManager = scenesManager;
	mWindow = window;
}

Plu::GameClient::~GameClient()
{
	ExitGame();
}

void Plu::GameClient::ExitGame()
{
	for (const auto& localPlayer : mLocalPlayers) {
		mObjectManager->DestroyObject(*localPlayer->GetEngineObjectHandle());
	}
	mLocalPlayers.Clear();
}

bool Plu::GameClient::IsCursorShown() const
{
	return mShowCursor;
}

void Plu::GameClient::HideCursor()
{
	mShowCursor = false;
	mWindow->SetCursorVisibility(false);
}

void Plu::GameClient::ShowCursor()
{
	mShowCursor = true;
	mWindow->SetCursorVisibility(true);
}

Plu::TUsePointer<Plu::GameLocalPlayer> Plu::GameClient::GetLocalPlayerByID(UInt16 id)
{
	return id < mLocalPlayers.Size() ? mLocalPlayers[id] : nullptr;
}

UInt16 Plu::GameClient::JoinGameLocally()
{
	EngineObjectHandle newPlayerHandle = mObjectManager->CreateObject<GameLocalPlayer>();
	TOwningPointer<GameLocalPlayer> newPlayer = mObjectManager->GetObjectAsOwner<GameLocalPlayer>(newPlayerHandle);
	UInt16 idx = mLocalPlayers.Size();
	newPlayer->Init(mScenesManager, idx);
	mLocalPlayers.PushBack(newPlayer);
	mScenesManager->GetCurrentWorld()->JoinPlayerLocally(idx);
	PLU_CORE_INFO("New player {}", idx);
	return idx;
}
