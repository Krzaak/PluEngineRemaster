//
// Created by Plutex on 2026-02-27.
//

#ifndef PLUENGINE_INPUTHANDLER_H
#define PLUENGINE_INPUTHANDLER_H

#include <functional>

#include "InputInfo.h"
#include "PluSTL_FWD.h"
#include "PluEngine/Core.h"

namespace Plu
{
	PLU_CLASS(PyExport, NoReflection)
	class PLU_API InputHandler final
	{
		GameHashMap<Key, std::function<void()>> mPressedActions;
		GameHashMap<Key, std::function<void()>> mReleasedActions;
		GameHashMap<Key, std::function<void()>> mHoldActions;
		GameHashMap<Key, ButtonState> mKeyboard;
	public:
		InputHandler() = default;
		~InputHandler();

		void TickHandler();
		void UpdateKeyState(Key key, ButtonState state);

		PLU_FUNCTION()
		void AddActionOnPress(Key key, std::function<void()> callback);
		PLU_FUNCTION()
		void AddActionOnRelease(Key key, std::function<void()> callback);
		PLU_FUNCTION()
		void AddActionOnHold(Key key, std::function<void()> callback);
	};
}

#endif //PLUENGINE_INPUTHANDLER_H