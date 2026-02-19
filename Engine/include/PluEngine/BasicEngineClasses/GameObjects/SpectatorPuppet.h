//
// Created by Plutex on 2026-02-19.
//

#ifndef PLUENGINE_SPECTATORPUPPET_H
#define PLUENGINE_SPECTATORPUPPET_H
#include "PluEngine/Core.h"
#include "PluEngine/GameCore/Puppet.h"
#include "SpectatorPuppet.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API SpectatorPuppet : public Puppet
	{
		REFLECTION_BODY_SPECTATORPUPPET()
	public:
		SpectatorPuppet() = default;
		~SpectatorPuppet() override = default;

		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SPECTATORPUPPET_H
