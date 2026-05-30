//
// Created by Plutex on 5/29/26.
//

#ifndef PLUENGINE_SCENEMANAGER_H
#define PLUENGINE_SCENEMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "SceneManager.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    class GameObject;
    class WorldComponent;
    class GameClient;
    class IRendererCamera;
    class Renderer;
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

        bool mIsInPIE = false;
#endif

        GameHashMap<String, TUsePointer<SceneInfo>> mRegisteredScenesByURL;

        //Helpers
        void UnloadScene(TUsePointer<SceneWorld> sceneWorld);
        void LoadScene(String url, TOwningPointer<SceneWorld>* field, bool play = true);

        TUsePointer<EngineObjectManager> mObjectManager;
        TUsePointer<Renderer> mRenderer;
        TUsePointer<GameClient> mClient;
        TUsePointer<EngineAssetManager> mAssetManager;
        TUsePointer<IShaderManager> mShaderManager;

        void DeserializeWorldComponent(JSON j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject);
        void LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld);
    public:
        SceneManager();
        virtual ~SceneManager() override;

        //Lifetime
        void Initialize(ApplicationInfo* appInfo);
        void OnUpdate(float deltaTime);

        //Getters
        String GetCurrentWorldName();
        TUsePointer<SceneWorld> GetCurrentWorld();

        //Utils
        bool ConnectToWorld(String URL, bool startPlayOnLoad = true); //URL can be SceneName or IP address
        bool IsAnySceneOpen();
        void RegisterSceneInfo(TUsePointer<SceneInfo> sceneInfo);
        void LoadGameObjectFromJSON(TUsePointer<SceneWorld> sceneWorld, JSON j);

#ifdef PLU_ENGINE_EDITOR_BUILD
        void SaveActiveScene();

        void CreateOverlayScene();
        void UnloadOverlayScene();

        bool EnterPIE();
        void ExitPIE();

        bool IsInPIE() const;
#endif
    };

    PLU_FUNCTION()
    PLU_API TUsePointer<SceneWorld> GetCurrentWorld();
}

#endif //PLUENGINE_SCENEMANAGER_H
