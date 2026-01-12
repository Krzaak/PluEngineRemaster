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
	class EditorScene : public SceneInfo
	{
		REFLECTION_BODY_EDITORSCENE()
	private:
		GameHashMap<MaxUInt64, TOwningPointer<GameObject>> mGameObjects;

		TUsePointer<EngineObjectManager> mEngineObjectManager;
	public:
		EditorScene();
		~EditorScene() override;

		void Init(const TUsePointer<EngineObjectManager> &engineObjectManager);

		DynamicArray<TUsePointer<GameObject>> GetAllGameObjects() override;
	};
}

#endif //PLUENGINE_EDITORSCENEIMPL_H
