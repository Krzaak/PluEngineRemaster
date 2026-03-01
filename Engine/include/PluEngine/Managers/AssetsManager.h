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
	struct PLU_API IAssetInfo
	{
		REFLECTION_BODY_IASSETINFO()

		PLU_PROPERTY()
		PluUUID Uuid;
	};

	PLU_CLASS(Abstract)
	class PLU_API IAssetManager : public EngineObject
	{
		REFLECTION_BODY_IASSETMANAGER()
	public:
		void InitAssetManagerForPython(TUsePointer<IAssetManager> assetManager);
		virtual TUsePointer<IAssetInfo> GetAssetByUUID(PluUUID uuid) = 0;
	};

	PLU_FUNCTION()
	PLU_API TUsePointer<IAssetInfo> GetAssetByUUID(UInt64 uuid);

	PLU_API TUsePointer<IAssetInfo> GetAssetUserAsRaw(IAssetInfo* assetInfo);
}

#endif //PLUENGINE_ASSETSMANAGER_H
