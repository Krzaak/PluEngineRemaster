//
// Created by Plutex on 2026-02-12.
//

#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/GameCore/GameLocalPlayer.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"

Plu::GameClient::GameClient(const TUsePointer<EngineObjectManager> &objectManager,
                            const TUsePointer<IScenesManager> &scenesManager, const TUsePointer<InputManager> &inputManager)
{
	mObjectManager = objectManager;
	mInputManager = inputManager;
	mScenesManager = scenesManager;
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
	IWindow::SetCursorVisibility(false);
}

void Plu::GameClient::ShowCursor()
{
	mShowCursor = true;
	IWindow::SetCursorVisibility(true);
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
