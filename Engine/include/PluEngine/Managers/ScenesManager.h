//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SCENESMANAGER_H
#define PLUENGINE_SCENESMANAGER_H
#include "AssetsManager.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ScenesManager.generated.h"
#include "SceneInfo.generated.h"
#include "PluSTL_FWD.h"

namespace Plu
{
	class GameObject;
	PLU_CLASS(Abstract)
	class PLU_API SceneInfo : public EngineObject, public IAssetInfo
	{
		REFLECTION_BODY_SCENEINFO()
	public:
		String URL;

		virtual DynamicArray<TUsePointer<GameObject>> GetAllGameObjects() {return DynamicArray<TUsePointer<GameObject>>();}
	};

	PLU_CLASS(Abstract)
	class PLU_API ScenesManager : public EngineObject
	{
		REFLECTION_BODY_SCENESMANAGER()
	public:
		virtual bool ConnectToWorld(String URL) = 0; //URL can be SceneName or IP address
		virtual String GetCurrentWorldName() = 0;
	};
}

#endif //PLUENGINE_SCENESMANAGER_H