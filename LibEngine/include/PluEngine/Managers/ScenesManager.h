//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SCENESMANAGER_H
#define PLUENGINE_SCENESMANAGER_H

#include "AssetsManager.h"
#include "PluSTL_FWD.h"
#include "ScenesManager.generated.h"

namespace Plu
{
	PLU_STRUCT()
	struct PLU_API SceneInfo : IAssetData
	{
		REFLECTION_BODY_SCENEINFO()

		PLU_PROPERTY()
		String URL;
	};
}

#endif //PLUENGINE_SCENESMANAGER_H