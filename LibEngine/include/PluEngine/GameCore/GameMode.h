//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_GAMEMODE_H
#define PLUENGINE_GAMEMODE_H
#include "PluEngine/Core.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "Controller.h"
#include "Puppet.h"
#include "GameMode.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API GameMode : public GameObject
	{
		REFLECTION_BODY_GAMEMODE()
	public:
		GameMode();
		~GameMode() override = default;

		void OnSetupComponents() override;

		PLU_PROPERTY(PyExport)
		TClassPointer<Controller> ControllerClass;

		PLU_PROPERTY(PyExport)
		TClassPointer<Puppet> PuppetClass;
	};
}

#endif //PLUENGINE_GAMEMODE_H
