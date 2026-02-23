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
	private:
		TUsePointer<Controller> mController;
		friend class Controller;
	public:
		Puppet() = default;
		~Puppet() override = default;

		TUsePointer<Controller> GetController() {return mController;}

		virtual void OnPossessed(TUsePointer<Controller> newController) {};
		virtual void OnUnpossessed() {};
	};
}

#endif //PLUENGINE_PUPPET_H
