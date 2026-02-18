//
// Created by Plutex on 2026-02-18.
//

#ifndef PLUENGINE_INPUTMANAGER_H
#define PLUENGINE_INPUTMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "InputManager.generated.h"
#include "PlatformInputBackend.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API InputManager final : public EngineObject
	{
		REFLECTION_BODY_INPUTMANAGER()
	private:
		TOwningPointer<PlatformInputBackend> mInputBackend;
	public:
		InputManager();
		~InputManager() override;

		TUsePointer<PlatformInputBackend> GetInputBackend();
	};
}

#endif //PLUENGINE_INPUTMANAGER_H
