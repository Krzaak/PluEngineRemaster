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