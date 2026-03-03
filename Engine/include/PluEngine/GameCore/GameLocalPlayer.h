//
// Created by Plutex on 2026-02-12.
//

#ifndef PLUENGINE_GAMELOCALPLAYER_H
#define PLUENGINE_GAMELOCALPLAYER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameLocalPlayer.generated.h"

struct MouseState;
enum class ButtonState : UInt8;
enum class Key : UInt16;

namespace Plu
{
	PLU_CLASS()
	class PLU_API GameLocalPlayer : public EngineObject
	{
		REFLECTION_BODY_GAMELOCALPLAYER()
	private:
		TUsePointer<IScenesManager> mScenesManager;
		UInt16 mLocalPlayerIndex;
	public:
		GameLocalPlayer() = default;
		~GameLocalPlayer() override = default;

		void Init(const TUsePointer<IScenesManager> &sceneManager, UInt16 id);
		void OnKeyboardKeyUpdate(Key key, ButtonState state);
		void OnMouseUpdate(MouseState& newState);
	};
}

#endif //PLUENGINE_GAMELOCALPLAYER_H
