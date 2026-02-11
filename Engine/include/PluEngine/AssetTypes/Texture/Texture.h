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
	struct PLU_API TextureInfo : IAssetInfo
	{
		REFLECTION_BODY_TEXTUREINFO()

		PLU_PROPERTY()
		Int32 Width;

		PLU_PROPERTY()
		Int32 Height;

		PLU_PROPERTY()
		Int32 Channels;

		unsigned char* Data;
	};
}

#endif //PLUENGINE_TEXTURE_H
