//
// Created by Plutex on 6/5/26.
//

#include "RuntimeShaderCode.h"

Plu::RuntimeShaderCode::RuntimeShaderCode(String lineFromArchive)
{
    String::SizeType semicolon = lineFromArchive.Find(';');
    String uuidStr = lineFromArchive.Substring(0, semicolon);
    PluUUID uuid = uuidStr.ToInt<UInt64>();
    Uuid = uuid;
    Name = uuidStr;
    mCode = lineFromArchive.Substring(semicolon + 1);
}

Plu::String Plu::RuntimeShaderCode::GetCode()
{
    return mCode;
}

DynamicArray<Plu::TOwningPointer<Plu::IShaderUniform>> * Plu::RuntimeShaderCode::GetCodeUniforms()
{
    return nullptr;
}

void Plu::RuntimeShaderCode::RenewUniforms()
{
}
