//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_CONTROLLER_H
#define PLUENGINE_CONTROLLER_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Input/InputHandler.h"
#include "Controller.generated.h"

namespace Plu
{
	class Puppet;
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API Controller : public GameObject
	{
		REFLECTION_BODY_CONTROLLER()
	private:
		TUsePointer<Puppet> mPossessedPuppet;
		UInt16 mPlayerID = -1;
		friend class SceneWorld;
		InputHandler mControllerInputHandler;
		//Human accessable
		Vec3 mControlRotation;
		//Processed to correct rotation based on mControlRotation, ready for use
		Vec3 mRealControlRotation;
	public:
		Controller() = default;
		~Controller() override = default;

		PLU_FUNCTION()
		void Possess(TUsePointer<Puppet> puppet);

		PLU_FUNCTION()
		void Unpossess();

		PLU_FUNCTION()
		[[nodiscard]] bool IsCursorShown() const;
		PLU_FUNCTION()
		void HideCursor();
		PLU_FUNCTION()
		void ShowCursor();

		PLU_FUNCTION()
		TUsePointer<Puppet> GetControllerPuppet();

		PLU_FUNCTION()
		InputHandler *GetInputHandler() override;

		PLU_FUNCTION()
		void SetControlRotation(Vec3 newRot);
		PLU_FUNCTION()
		Vec3 GetControlRotation() const;
		Vec3 GetControlRotationForPuppet() const;
	};
}

#endif //PLUENGINE_CONTROLLER_H
