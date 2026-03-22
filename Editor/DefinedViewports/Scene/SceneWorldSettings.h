//
// Created by Plutex on 2026-02-15.
//

#ifndef PLUENGINE_SCENEWORLDSETTINGS_H
#define PLUENGINE_SCENEWORLDSETTINGS_H
#include "EditorViewports/IEditorPanel.h"
#include "PluEngine/Core.h"
#include "SceneWorldSettings.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SceneWorldSettings : public IEditorPanel
	{
		REFLECTION_BODY_SCENEWORLDSETTINGS()
	public:
		SceneWorldSettings() = default;
		virtual ~SceneWorldSettings() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SCENEWORLDSETTINGS_H