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
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API Puppet : public GameObject
	{
		REFLECTION_BODY_PUPPET()
	private:
		TUsePointer<Controller> mController;
		friend class Controller;
	public:
		Puppet() = default;
		~Puppet() override = default;

		PLU_FUNCTION()
		TUsePointer<Controller> GetController() {return mController;}

		PLU_FUNCTION(PyOverride)
		virtual void OnPossessed(TUsePointer<Controller> newController) {};

		PLU_FUNCTION(PyOverride)
		virtual void OnUnpossessed() {};
	};
}

#endif //PLUENGINE_PUPPET_H
