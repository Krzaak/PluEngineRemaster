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
        // cloneSource, when given, populates the new world by duplicating that world's objects in
        // memory instead of reading the scene asset off disk — that is how PIE starts from the
        // editor world's live state.
        void LoadScene(String url, TOwningPointer<SceneWorld>* field, bool play = true, TUsePointer<SceneWorld> cloneSource = nullptr);

        TUsePointer<EngineObjectManager> mObjectManager;
        TUsePointer<GameClient> mClient;
        TUsePointer<EngineAssetManager> mAssetManager;
        TUsePointer<IShaderManager> mShaderManager;

        // One context is built per scene load and threaded through every object/component below it —
        // it holds nothing per-object, and allocating one per component was pure overhead.
        DeserializationContext MakeDeserializationContext();

        void DeserializeWorldComponent(DeserializationContext* dc, const JSON& j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject);
        void LoadGameObjectFromJSON(DeserializationContext* dc, TUsePointer<SceneWorld> sceneWorld, const JSON& j);
        void LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld);
        void LoadSceneFromJson(TUsePointer<SceneWorld> sceneWorld, const JSON& j);

        // In-memory duplication of a live world, used instead of a serialize/deserialize round-trip.
        // Mirrors the JSON path step for step (spawn, copy properties, match components by name,
        // recurse into the component tree) so a cloned world and a loaded one come out the same.
        void CloneSceneInto(TUsePointer<SceneWorld> target, TUsePointer<SceneWorld> source);
        void CloneGameObjectInto(TUsePointer<SceneWorld> targetWorld, TUsePointer<GameObject> source);
        void CloneWorldComponent(TUsePointer<WorldComponent> source, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> targetObject);
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
        // Full JSON of the active scene. base carries any keys of the scene asset we do not own and
        // must not drop (SaveActiveScene passes the file's current contents); PIE passes an empty
        // one, since it only ever reads back the keys written here.
        JSON SerializeActiveScene(JSON base);
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
