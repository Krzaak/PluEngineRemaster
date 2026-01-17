//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SCENESMANAGER_H
#define PLUENGINE_SCENESMANAGER_H
#include "AssetsManager.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ScenesManager.generated.h"
#include "SceneWorld.generated.h"
#include "PluSTL_FWD.h"

namespace Plu
{
	class GameObject;
	struct PLU_API SceneInfo : IAssetInfo
	{
		String URL;
	};

	PLU_CLASS(Abstract)
	class PLU_API SceneWorld : public EngineObject
	{
		REFLECTION_BODY_SCENEWORLD()
	public:
		virtual TUsePointer<GameObject> SpawnGameObject(TypeInfo* objectClass) = 0;
		virtual DynamicArray<TUsePointer<GameObject>> GetAllGameObjects() = 0;
	};

	PLU_CLASS(Abstract)
	class PLU_API ScenesManager : public EngineObject
	{
		REFLECTION_BODY_SCENESMANAGER()
	public:
		virtual bool ConnectToWorld(String URL) = 0; //URL can be SceneName or IP address
		virtual String GetCurrentWorldName() = 0;
		virtual bool IsAnySceneOpen() = 0;
	};
}

#endif //PLUENGINE_SCENESMANAGER_H