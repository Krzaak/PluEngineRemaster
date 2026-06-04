//
// Created by Plutex on 5/27/26.
//

#ifndef PLUENGINE_RUNTIMEAPP_H
#define PLUENGINE_RUNTIMEAPP_H

#include "PluEngine/Application.h"

namespace Plu
{
    struct GameStartupSettings;

    class RuntimeApp : public Application
    {
        TOwningPointer<GameStartupSettings> mGameStartupSettings;
    public:
        RuntimeApp();
        virtual ~RuntimeApp() override;

        void OnImGuiRender() override;
        void OnInit() override;
        void OnPostInit() override;
        void OnShutdown() override;
        void OnTick(float deltaTime) override;
    };
}



#endif //PLUENGINE_RUNTIMEAPP_H
