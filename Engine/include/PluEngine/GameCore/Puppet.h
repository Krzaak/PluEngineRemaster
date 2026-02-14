//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_PUPPET_H
#define PLUENGINE_PUPPET_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "Puppet.generated.h"

namespace Plu
{
	class Controller;
	PLU_CLASS()
	class PLU_API Puppet : public GameObject
	{
		REFLECTION_BODY_PUPPET()
	public:
		Puppet() = default;
		~Puppet() override = default;

		virtual void OnPossessed(TUsePointer<Controller> newController) {};
		virtual void OnUnpossessed() {};
	};
}

#endif //PLUENGINE_PUPPET_H
