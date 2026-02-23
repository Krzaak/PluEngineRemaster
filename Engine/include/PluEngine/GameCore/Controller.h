//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_CONTROLLER_H
#define PLUENGINE_CONTROLLER_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObject.h"
#include "Controller.generated.h"
#include "PluEngine/Input/InputInfo.h"

namespace Plu
{
	class Puppet;
	PLU_CLASS()
	class PLU_API Controller : public GameObject
	{
		REFLECTION_BODY_CONTROLLER()
	private:
		TUsePointer<Puppet> mPossessedPuppet;
		UInt16 mPlayerID;
		friend class SceneWorld;
	protected:
		bool IsKeyboardKeyDown(Key key);
	public:
		Controller() = default;
		~Controller() override = default;

		void Possess(TUsePointer<Puppet> puppet);
		void Unpossess();

		bool IsKeyboardKeyDown(Key key, const TUsePointer<Puppet> &puppet);

		TUsePointer<Puppet> GetControllerPuppet();
	};
}

#endif //PLUENGINE_CONTROLLER_H
