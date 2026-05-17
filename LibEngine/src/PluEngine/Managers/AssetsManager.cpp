//
// Created by Plutex on 2026-02-24.
//

#include "PluEngine/Managers/AssetsManager.h"

static Plu::TUsePointer<Plu::IAssetManager> gAssetManager;

void Plu::IAssetManager::InitAssetManagerForPython(TUsePointer<IAssetManager> assetManager)
{
	gAssetManager = assetManager;
}

Plu::TUsePointer<Plu::IAssetInfo> Plu::GetAssetByUUID(UInt64 uuid)
{
	return gAssetManager->GetAssetByUUID(uuid);
}

Plu::TUsePointer<Plu::IAssetInfo> Plu::GetAssetUserAsRaw(IAssetInfo *assetInfo)
{
	return gAssetManager->GetAssetByUUID(assetInfo->Uuid);
}
