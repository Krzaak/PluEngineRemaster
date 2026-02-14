//
// Created by Plutex on 2026-02-12.
//

#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/GameCore/GameLocalPlayer.h"
#include "PluEngine/GameCore/GameMode.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

Plu::GameClient::GameClient(const TUsePointer<EngineObjectManager> &objectManager, const TUsePointer<IScenesManager> &scenesManager)
{
	mObjectManager = objectManager;
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

UInt16 Plu::GameClient::JoinGameLocally()
{
	EngineObjectHandle newPlayerHandle = mObjectManager->CreateObject<GameLocalPlayer>();
	TOwningPointer<GameLocalPlayer> newPlayer = mObjectManager->GetObjectAsOwner<GameLocalPlayer>(newPlayerHandle);
	UInt16 idx = mLocalPlayers.Size();
	mLocalPlayers.PushBack(newPlayer);
	//mScenesManager->GetCurrentWorld()->SpawnGameObject(); TODO finish this player controller here
	PLU_CORE_INFO("New player {}", idx);
	return idx;
}
