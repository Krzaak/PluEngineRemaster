//
// Created by Plutex on 1/13/26.
//

#ifndef PLUENGINE_SCENEVIEWPORT_H
#define PLUENGINE_SCENEVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "SceneViewport.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SceneViewport : public IEditorViewport
	{
		REFLECTION_BODY_SCENEVIEWPORT()
	private:
		EventHandle mGameObjectsChangedHandle;
		EventHandle mNewPythonTypeHandle;

		// Python classes re-registered since the last frame (script hot reload). Collected instead of
		// acted upon right away: "NewPythonType" is dispatched from RegisterPluClass, i.e. in the middle
		// of RunProjectScripts, when the other project modules are already popped out of sys.modules and
		// not re-imported yet — constructing a python object there can raise ImportError, and every
		// class of the batch would kick off its own full recreate pass.
		DynamicArray<String> mPendingPythonTypeReloads;

		void FlushPendingPythonTypeReloads();
	public:
		SceneViewport() = default;
		virtual ~SceneViewport() override = default;

		bool UsesEditorCamera() const override { return true; }

		void OnInit() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SCENEVIEWPORT_H