//
// Created by Plutex on 1/12/26.
//

#ifndef PLUENGINE_EDITORSCENEIMPL_H
#define PLUENGINE_EDITORSCENEIMPL_H
#include "PluEngine/Core.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "EditorScene.generated.h"

namespace Plu
{
	class EngineObjectManager;

	PLU_CLASS()
	class EditorScene : public SceneWorld
	{
		REFLECTION_BODY_EDITORSCENE()
	private:
		GameHashMap<MaxUInt64, TOwningPointer<GameObject>> mGameObjects;

		TUsePointer<EngineObjectManager> mEngineObjectManager;
	public:
		EditorScene();
		virtual ~EditorScene() override;

		SceneInfo Info;

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager);

		void LoadGameObjects();
		void UnloadGameObjects();
		void Play();

		TUsePointer<GameObject> SpawnGameObject(TypeInfo *objectClass) override;
		DynamicArray<TUsePointer<GameObject>> GetAllGameObjects() override;
	};
}

#endif //PLUENGINE_EDITORSCENEIMPL_H
