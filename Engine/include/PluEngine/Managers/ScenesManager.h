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
	class EngineObjectManager;

	struct PLU_API SceneInfo : IAssetInfo
	{
		String URL;
	};

	PLU_CLASS()
	class PLU_API SceneWorld : public EngineObject
	{
		REFLECTION_BODY_SCENEWORLD()
	protected:
		GameHashMap<MaxUInt64, TOwningPointer<GameObject>> mGameObjects;
		TUsePointer<EngineObjectManager> mEngineObjectManager;
	public:
		SceneWorld() = default;
		virtual ~SceneWorld() override;

		SceneInfo Info;

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager);

		void LoadGameObjects();
		void UnloadGameObjects();
		void Play();

		virtual TUsePointer<GameObject> SpawnGameObject(TypeInfo* objectClass);
		virtual DynamicArray<TUsePointer<GameObject>> GetAllGameObjects();
		void GetFormattedGameObjectNames(DynamicArray<String>* result);
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