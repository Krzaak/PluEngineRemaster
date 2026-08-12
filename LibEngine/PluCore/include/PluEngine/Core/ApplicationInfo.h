//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_APPLICATIONINFO_H
#define PLUENGINE_APPLICATIONINFO_H
#include <PluSTL_FWD.h>

#include "PluEngine/Core.h"

namespace Plu
{
    struct DeserializationContext;
    class EngineAssetManager;
    class InputManager;
    class GameClient;
    class RenderingManager;
    class IAssetManager;
    class IShaderManager;
    class SceneManager;
    class EngineObjectManager;
    class IWindow;
    class WindowsManager;
    class IPythonManager;

    // Every major subsystem, in one bundle passed down to whatever needs a sibling. It lives in
    // PluCore rather than beside Application because half the engine takes an ApplicationInfo*
    // parameter, and a type from the top layer cannot be named by the layers below it.
    //
    // Only forward declarations are needed here: the struct holds nothing but TUsePointers, so no
    // subsystem header comes with it. Application still owns the instance and fills it in.
    struct PLUCORE_API ApplicationInfo
    {
        // Alias of window 0; WindowsManager owns the full list.
        TUsePointer<IWindow> AppWindow;
        TUsePointer<WindowsManager> AppWindowsManager;
        TUsePointer<EngineObjectManager> AppObjectManager;
        TUsePointer<SceneManager> AppScenesManager;
        TUsePointer<IShaderManager> AppShaderManager;
        TUsePointer<EngineAssetManager> AppAssetManager;
        TUsePointer<RenderingManager> AppRenderingManager;
        TUsePointer<InputManager> AppInputManager;
        TUsePointer<IPythonManager> AppPythonManager;

        TUsePointer<GameClient> Client;

        [[nodiscard]] DeserializationContext* ConstructDeserializationContext() const;
    };
}

#endif //PLUENGINE_APPLICATIONINFO_H
