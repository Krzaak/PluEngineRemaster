//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_PUPPET_H
#define PLUENGINE_PUPPET_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "Puppet.generated.h"
#include "PluEngine/Input/InputHandler.h"

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
		InputHandler mPuppetInputHandler;
	public:
		Puppet() = default;
		~Puppet() override = default;

		PLU_FUNCTION()
		TUsePointer<Controller> GetController();

		PLU_FUNCTION(PyOverride)
		virtual void OnPossessed(TUsePointer<Controller> newController) {};

		PLU_FUNCTION(PyOverride)
		virtual void OnUnpossessed() {};

		// Offset added to a PlayerStart location when this puppet spawns there. Lets a PlayerStart
		// mark the ground the puppet stands on instead of the puppet's origin (which for a capsule
		// body sits at its center, i.e. half a body height below the feet).
		PLU_FUNCTION(PyOverride)
		virtual Vec3 GetSpawnOffset() const;

		PLU_FUNCTION()
		InputHandler *GetInputHandler() override;
	};
}

#endif //PLUENGINE_PUPPET_H
