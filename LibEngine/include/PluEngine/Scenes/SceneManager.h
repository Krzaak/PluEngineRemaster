//
// Created by Plutex on 5/29/26.
//

#ifndef PLUENGINE_SCENEMANAGER_H
#define PLUENGINE_SCENEMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "SceneManager.generated.h"

namespace Plu
{
    struct SceneInfo;
    class SceneWorld;
    PLU_CLASS()
    class PLU_API SceneManager : public EngineObject
    {
        REFLECTION_BODY_SCENEMANAGER()
    private:
        TOwningPointer<SceneWorld> mActiveScene;
#ifdef PLU_ENGINE_EDITOR_BUILD
        TOwningPointer<SceneWorld> mOverlayScene;
        TOwningPointer<SceneWorld> mActivePIEScene;
#endif

        GameHashMap<String, TUsePointer<SceneInfo>> mRegisteredScenesByURL;

        //Helpers
        void UnloadScene(String url);
        void LoadScene(String url, bool play = true);
#ifdef PLU_ENGINE_EDITOR_BUILD
        void CreateOverlayScene();
        void UnloadOverlayScene();

        void EnterPIE();
        void ExitPIE();
#endif

        TUsePointer<EngineObjectManager> mObjectManager;
    public:
        SceneManager();
        virtual ~SceneManager() override;

        //Lifetime
        void Initialize(TUsePointer<EngineObjectManager> engineObjectManager);
        void OnUpdate(float deltaTime);

        //Getters
        String GetCurrentWorldName();
        TUsePointer<SceneWorld> GetCurrentWorld();

        //Utils
        bool ConnectToWorld(String URL); //URL can be SceneName or IP address
        bool IsAnySceneOpen();
        void RegisterSceneInfo(TUsePointer<SceneInfo> sceneInfo);
    };

    PLU_FUNCTION()
    PLU_API TUsePointer<SceneWorld> GetCurrentWorld();
}

#endif //PLUENGINE_SCENEMANAGER_H
