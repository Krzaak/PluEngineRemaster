//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_ASSETSMANAGER_H
#define PLUENGINE_ASSETSMANAGER_H
#include "PluEngine/Objects/EngineObject.h"
#include "IAssetManager.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	class PluUUID;

	struct PLU_API IAssetInfo
	{
		PluUUID Uuid;
	};
	PLU_CLASS(Abstract)
	class PLU_API IAssetManager : public EngineObject
	{
		REFLECTION_BODY_IASSETMANAGER()
	public:
		virtual IAssetInfo* GetAssetByUUID(PluUUID uuid) = 0;
	};
}

#endif //PLUENGINE_ASSETSMANAGER_H
