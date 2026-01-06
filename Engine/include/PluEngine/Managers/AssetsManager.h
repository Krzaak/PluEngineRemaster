//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_ASSETSMANAGER_H
#define PLUENGINE_ASSETSMANAGER_H
#include "PluEngine/Objects/EngineObject.h"
#include "IAssetManager.generated.h"

namespace Plu
{
	class PluUUID;

	struct IAssetInfo
	{

	};
	PLU_CLASS(Abstract)
	class IAssetManager : public EngineObject
	{
		REFLECTION_BODY_IASSETMANAGER()
	public:
		virtual bool Init(StringW PathToProject) = 0;
		virtual IAssetInfo* GetAssetByUUID(PluUUID uuid) = 0;
		virtual bool Shutdown() = 0;
	};
}

#endif //PLUENGINE_ASSETSMANAGER_H
