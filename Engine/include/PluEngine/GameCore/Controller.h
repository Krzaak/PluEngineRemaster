//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_CONTROLLER_H
#define PLUENGINE_CONTROLLER_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObject.h"
#include "Controller.generated.h"

namespace Plu
{
	class Puppet;
	PLU_CLASS()
	class PLU_API Controller : public GameObject
	{
		REFLECTION_BODY_CONTROLLER()
	private:
		TUsePointer<Puppet> mPossessedPuppet;
	public:
		Controller() = default;
		~Controller() override = default;

		void Possess(TUsePointer<Puppet> puppet);
		void Unpossess();

		TUsePointer<Puppet> GetControllerPuppet();
	};
}

#endif //PLUENGINE_CONTROLLER_H
