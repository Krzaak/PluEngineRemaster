//
// Created by Plutex on 08.01.2026.
//

#ifndef PLUENGINE_ENGINE_H
#define PLUENGINE_ENGINE_H
#include "Core.h"
#include "Objects/EngineObject.h"
#include "Engine.generated.h"
struct ImGuiContext;

namespace Plu
{
    //Global Singleton state for smth that links Engine
    PLU_CLASS()
    class PLU_API Engine final : public EngineObject
    {
        REFLECTION_BODY_ENGINE()
    private:
        ImGuiContext* mImGuiContext = nullptr;
    public:
        static void CreateEngine();
        static Engine* GetEngine();
        static void DestroyEngine();
        void InitImGui(ImGuiContext* ctx);
        ImGuiContext* GetImGuiContext() const;
    };
}

#endif //PLUENGINE_ENGINE_H