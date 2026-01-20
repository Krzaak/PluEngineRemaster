//
// Created by Plutex on 1/20/26.
//

#include "PluEngine/Shaders/ShaderCacheWriter.h"

Plu::TOwningPointer<Plu::IShaderCacheWriter> gShaderCacheWriter;

Plu::TUsePointer<Plu::IShaderCacheWriter> Plu::GetGlobalShaderCacheWriter()
{
	return gShaderCacheWriter;
}

void Plu::SetGlobalShaderCacheWriter(const TOwningPointer<IShaderCacheWriter> &newShaderCacheWriter)
{
	gShaderCacheWriter = newShaderCacheWriter;
}
