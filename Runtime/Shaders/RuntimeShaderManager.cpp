//
// Created by Plutex on 6/5/26.
//

#include "RuntimeShaderManager.h"
#include "RuntimeShaderCode.h"
#include "PluEngine/PluUtils.h"

Plu::RuntimeShaderManager::RuntimeShaderManager()
{
}

Plu::RuntimeShaderManager::~RuntimeShaderManager()
{
}

void Plu::RuntimeShaderManager::ShaderCodeScan()
{
    Path pathToSelf = GetExePath().GetParentPath().ToString().ToNarrow();
    pathToSelf += "/Shaders.txt";
    std::ifstream ifs(pathToSelf.CStr());
    std::string line;
    while (std::getline(ifs, line)) {
        TOwningPointer<RuntimeShaderCode> newShaderCode = CreateOwning<RuntimeShaderCode>(line.c_str());
        mShaderCodes.Insert(newShaderCode->Uuid, newShaderCode);
        PLU_TRACE("New Shader Code Registered! UUID {}", newShaderCode->Uuid.getUUID());
    }
}

bool Plu::RuntimeShaderManager::ShaderCodeExists(PluUUID uuid)
{
    return mShaderCodes.Contains(uuid);
}

DynamicArray<Plu::TUsePointer<Plu::ShaderProgram>> * Plu::RuntimeShaderManager::GetRenderableShaderPrograms()
{
    return nullptr;
}

Plu::TUsePointer<Plu::IShaderCode> Plu::RuntimeShaderManager::GetShaderCode(PluUUID uuid)
{
    if (ShaderCodeExists(uuid)) {
        return mShaderCodes[uuid];
    }
    return nullptr;
}

Plu::TUsePointer<Plu::ShaderProgram> Plu::RuntimeShaderManager::GetShaderProgram(PluUUID uuid)
{
    return nullptr;
}

void Plu::RuntimeShaderManager::LoadShader(PluUUID uuid)
{
}
