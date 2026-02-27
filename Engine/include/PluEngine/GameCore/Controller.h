//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_CONTROLLER_H
#define PLUENGINE_CONTROLLER_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObject.h"
#include "Controller.generated.h"
#include "PluEngine/Input/InputHandler.h"
#include "PluEngine/Input/InputInfo.h"

namespace Plu
{
	class Puppet;
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API Controller : public GameObject
	{
		REFLECTION_BODY_CONTROLLER()
	private:
		TUsePointer<Puppet> mPossessedPuppet;
		UInt16 mPlayerID;
		friend class SceneWorld;
		InputHandler mControllerInputHandler;
	public:
		Controller() = default;
		~Controller() override = default;

		PLU_FUNCTION()
		void Possess(TUsePointer<Puppet> puppet);

		PLU_FUNCTION()
		void Unpossess();

		PLU_FUNCTION()
		TUsePointer<Puppet> GetControllerPuppet();

		PLU_FUNCTION()
		InputHandler *GetInputHandler() override;
	};
}

#endif //PLUENGINE_CONTROLLER_H
