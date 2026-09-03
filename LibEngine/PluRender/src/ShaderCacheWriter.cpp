//
// Created by Plutex on 1/20/26.
//

#include "PluEngine/Render/ShaderCacheWriter.h"

Plu::TUsePointer<Plu::IShaderCacheWriter> gShaderCacheWriter;

Plu::TUsePointer<Plu::IShaderCacheWriter> Plu::GetGlobalShaderCacheWriter()
{
	return gShaderCacheWriter;
}

void Plu::SetGlobalShaderCacheWriter(const TUsePointer<IShaderCacheWriter> &newShaderCacheWriter)
{
	gShaderCacheWriter = newShaderCacheWriter;
}
