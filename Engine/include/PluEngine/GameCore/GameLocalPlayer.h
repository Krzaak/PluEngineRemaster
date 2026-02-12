//
// Created by Plutex on 2026-02-12.
//

#ifndef PLUENGINE_GAMELOCALPLAYER_H
#define PLUENGINE_GAMELOCALPLAYER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameLocalPlayer.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API GameLocalPlayer : public EngineObject
	{
		REFLECTION_BODY_GAMELOCALPLAYER()
	public:
		GameLocalPlayer() = default;
		~GameLocalPlayer() override = default;
	};
}

#endif //PLUENGINE_GAMELOCALPLAYER_H
