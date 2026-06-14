//
// Created by Plutex on 1/13/26.
//

#ifndef PLUENGINE_SCENEVIEWPORT_H
#define PLUENGINE_SCENEVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "StaticMeshViewport.generated.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/GameObject/GameObject.h"

namespace Plu
{
	PLU_CLASS()
	class EditorMeshObject : public GameObject
	{
		REFLECTION_BODY_EDITORMESHOBJECT()
	public:
		EditorMeshObject() = default;
		virtual ~EditorMeshObject() override = default;

		void OnSetupComponents() override;

		TUsePointer<StaticMeshComponent> MeshComponent;
	};

	struct MaterialInfo;
	PLU_CLASS()
	class StaticMeshViewport : public IEditorViewport
	{
		REFLECTION_BODY_STATICMESHVIEWPORT()
	private:
	public:
		StaticMeshViewport() = default;
		virtual ~StaticMeshViewport() override = default;

		TUsePointer<MaterialInfo> Material;
		bool ShowCollision = false;
		bool CollisionDirty = false;

		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SCENEVIEWPORT_H