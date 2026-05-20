//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_ASSETSMANAGER_H
#define PLUENGINE_ASSETSMANAGER_H
#include "PluEngine/Objects/EngineObject.h"
#include "AssetsManager.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	class PluUUID;

	PLU_STRUCT(UUID=Uuid, PyExport)
	struct PLU_API IAssetData
	{
		REFLECTION_BODY_IASSETDATA()

		PLU_PROPERTY()
		PluUUID Uuid;
	};

	PLU_FUNCTION(PyName=GetAssetByUUID)
	PLU_API TUsePointer<IAssetData> GetAssetByUUID(UInt64 uuid);

	PLU_API TUsePointer<IAssetData> GetAssetUserAsRaw(IAssetData* assetInfo);
}

#endif //PLUENGINE_ASSETSMANAGER_H
