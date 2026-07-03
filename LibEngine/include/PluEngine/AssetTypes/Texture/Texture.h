//
// Created by Plutex on 2026-02-07.
//

#ifndef PLUENGINE_TEXTURE_H
#define PLUENGINE_TEXTURE_H
#include "PluEngine/Core.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "Texture.generated.h"

namespace Plu
{
	PLU_STRUCT()
	struct PLU_API TextureInfo : IAssetData
	{
		REFLECTION_BODY_TEXTUREINFO()

		// Owns the pixel buffer (always allocated with new[] in TextureImport::LoadTexture;
		// the stbi-decoded import path frees its data itself and never lands here).
		~TextureInfo() override { delete[] Data; }

		PLU_PROPERTY()
		Int32 Width = 0;

		PLU_PROPERTY()
		Int32 Height = 0;

		PLU_PROPERTY()
		Int32 Channels = 0;

		unsigned char* Data = nullptr;
	};
}

#endif //PLUENGINE_TEXTURE_H
