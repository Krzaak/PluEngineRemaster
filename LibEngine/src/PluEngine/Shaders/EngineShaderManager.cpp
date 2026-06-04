//
// Created by Plutex on 6/5/26.
//

#include "PluEngine/Shaders/EngineShaderManager.h"


Plu::EngineShaderManager::EngineShaderManager()
{
}

Plu::EngineShaderManager::~EngineShaderManager()
{
}

Plu::TUsePointer<Plu::IShaderCode> Plu::EngineShaderManager::GetShaderCode(PluUUID uuid)
{
    if (ShaderCodeExists(uuid)) {
        return mShaderCodes[uuid];
    }
    return nullptr;
}

Plu::TUsePointer<Plu::ShaderProgram> Plu::EngineShaderManager::GetShaderProgram(PluUUID uuid)
{
    if (ShaderProgramExists(uuid)) {
        return mShaderPrograms[uuid];
    }
    return nullptr;
}

Plu::HashSet<UInt64> * Plu::EngineShaderManager::GetShaderProgramsInUse(PluUUID uuid)
{
    return &mLoadedShaderPrograms;
}

bool Plu::EngineShaderManager::ShaderCodeExists(PluUUID uuid) const
{
    return mShaderCodes.Contains(uuid);
}

bool Plu::EngineShaderManager::ShaderProgramExists(PluUUID uuid) const
{
    return mShaderPrograms.Contains(uuid);
}

bool Plu::EngineShaderManager::IsShaderProgramLoaded(PluUUID uuid) const
{
    return mLoadedShaderPrograms.Contains(uuid);
}
