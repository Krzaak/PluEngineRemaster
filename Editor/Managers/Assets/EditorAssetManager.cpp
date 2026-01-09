//
// Created by Plutex on 1/4/26.
//

#include "EditorAssetManager.h"

#include "PluEngine/PluUUID.h"

Plu::EditorAssetManager::EditorAssetManager()
{
}

Plu::EditorAssetManager::~EditorAssetManager()
{
}

Plu::IAssetInfo * Plu::EditorAssetManager::GetAssetByUUID(PluUUID uuid)
{
    return nullptr;
}

bool Plu::EditorAssetManager::Init(StringW PathToProject)
{
    return true;
}

bool Plu::EditorAssetManager::Shutdown()
{
    return true;
}
