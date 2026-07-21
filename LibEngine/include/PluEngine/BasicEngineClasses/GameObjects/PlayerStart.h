//
// Created by Plutex on 7/22/26.
//

#ifndef PLUENGINE_PLAYERSTART_H
#define PLUENGINE_PLAYERSTART_H

#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PlayerStart.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PLU_API PlayerStart : public GameObject
    {
        REFLECTION_BODY_PLAYERSTART()
    public:
        PlayerStart() = default;
        virtual ~PlayerStart() override = default;
    };
}

#endif //PLUENGINE_PLAYERSTART_H
