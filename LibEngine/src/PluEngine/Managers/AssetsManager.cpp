//
// Created by Plutex on 2026-02-24.
//

#include "PluEngine/Managers/AssetsManager.h"

static Plu::TUsePointer<Plu::IAssetManager> gAssetManager;

void Plu::IAssetManager::InitAssetManagerForPython(TUsePointer<IAssetManager> assetManager)
{
	gAssetManager = assetManager;
}

Plu::TUsePointer<Plu::IAssetData> Plu::GetAssetByUUID(UInt64 uuid)
{
	return gAssetManager->GetAssetByUUID(uuid);
}

Plu::TUsePointer<Plu::IAssetData> Plu::GetAssetUserAsRaw(IAssetData *assetInfo)
{
	return gAssetManager->GetAssetByUUID(assetInfo->Uuid);
}
