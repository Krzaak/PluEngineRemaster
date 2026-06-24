//
// Created by Plutex on 5/29/26.
//

#ifndef PLUENGINE_SCENEMANAGER_H
#define PLUENGINE_SCENEMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "SceneManager.generated.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Reflection/ClassPointer.h"

namespace Plu
{
    class IModifiableCamera;
    class GameObject;
    class WorldComponent;
    class GameClient;
    class IRendererCamera;
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

        // Kamera, z której renderuje się viewport edytora poza PIE. Dawniej przekazywana do
        // Renderer::SetCamera; w modelu snapshot RenderSnapshotBuilder pobiera ją stąd, gdy
        // nie ma kontrolera/pucharka. Nie-właścicielski wskaźnik (właścicielem jest edytor).
        IRendererCamera* mEditorCamera = nullptr;
#endif

        GameHashMap<String, TUsePointer<SceneInfo>> mRegisteredScenesByURL;

        //Helpers
        void UnloadScene(TUsePointer<SceneWorld> sceneWorld);
        void LoadScene(String url, TOwningPointer<SceneWorld>* field, bool play = true);

        TUsePointer<EngineObjectManager> mObjectManager;
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
        void DisconnectFromWorld();
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
        TUsePointer<SceneWorld> GetBaseSceneWorld();

        void SetEditorRenderCamera(IRendererCamera* camera);
        IRendererCamera* GetEditorRenderCamera() const;
#endif
    };

    PLU_FUNCTION()
    PLU_API TUsePointer<SceneWorld> GetCurrentWorld();
}

#endif //PLUENGINE_SCENEMANAGER_H
