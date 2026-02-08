//
// Created by Plutex on 2026-02-07.
//

#ifndef PLUENGINE_TEXTUREIMPORTER_H
#define PLUENGINE_TEXTUREIMPORTER_H
#include "Path/Path.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"

namespace Plu
{
	namespace TextureImport
	{
		bool ImportTexture(const PathW& origin, const PathW &outPath);
		void LoadTexture(const PathW &textPath, TextureInfo* textureInfo);
	}
}

#endif //PLUENGINE_TEXTUREIMPORTER_H