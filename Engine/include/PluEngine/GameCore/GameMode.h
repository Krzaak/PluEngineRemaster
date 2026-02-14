//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_GAMEMODE_H
#define PLUENGINE_GAMEMODE_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "Controller.h"
#include "Puppet.h"
#include "GameMode.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API GameMode : public GameObject
	{
		REFLECTION_BODY_GAMEMODE()
	public:
		GameMode();
		~GameMode() override = default;

		PLU_PROPERTY()
		TClassPointer<Controller> ControllerClass;

		PLU_PROPERTY()
		TClassPointer<Puppet> PuppetClass;
	};
}

#endif //PLUENGINE_GAMEMODE_H
