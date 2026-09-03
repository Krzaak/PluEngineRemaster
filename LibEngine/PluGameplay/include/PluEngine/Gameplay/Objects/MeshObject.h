//
// Created by Plutex on 2026-03-18.
//

#ifndef PLUENGINE_MESHOBJECT_H
#define PLUENGINE_MESHOBJECT_H
#include "PluEngine/Core.h"
#include "PluEngine/Gameplay/GameObject.h"
#include "MeshObject.generated.h"

namespace Plu
{
	class PhysicsBoxComponent;
	class StaticMeshComponent;
	PLU_CLASS()
	class PLUGAMEPLAY_API MeshObject : public GameObject
	{
		REFLECTION_BODY_MESHOBJECT()
	public:
		MeshObject() = default;
		virtual ~MeshObject() override = default;

		TUsePointer<StaticMeshComponent> MeshComponent;
		TUsePointer<PhysicsBoxComponent> BoxComponent;

		void OnSetupComponents() override;
	};
}

#endif //PLUENGINE_MESHOBJECT_H
