//
// Created by Plutex on 5/29/26.
//

#include "PluEngine/Scenes/SceneManager.h"

#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Scenes/SceneWorld.h"

Plu::SceneManager::SceneManager()
{

}

Plu::SceneManager::~SceneManager()
{
}

static Plu::TUsePointer<Plu::SceneManager> gSceneManager;

void Plu::SceneManager::Initialize(TUsePointer<EngineObjectManager> engineObjectManager)
{
    mObjectManager = engineObjectManager;
    mRegisteredScenesByURL["Overlay"] = nullptr;
    gSceneManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
}

void Plu::SceneManager::OnUpdate(float deltaTime)
{
    if (GetCurrentWorld()) GetCurrentWorld()->TickScene(deltaTime);
}

Plu::String Plu::SceneManager::GetCurrentWorldName()
{
    if (GetCurrentWorld()) {
        if (GetCurrentWorld()->Info) {
            return GetCurrentWorld()->Info->URL;
        }
    }
    return "";
}

Plu::TUsePointer<Plu::SceneWorld> Plu::SceneManager::GetCurrentWorld()
{
#ifdef PLU_ENGINE_EDITOR_BUILD
    return mOverlayScene ? mOverlayScene : mActivePIEScene ? mActivePIEScene : mActiveScene;
#else
    return mActiveScene;
#endif
}

bool Plu::SceneManager::ConnectToWorld(String URL)
{
#ifdef PLU_ENGINE_EDITOR_BUILD
    UnloadOverlayScene();
    if (URL == "Overlay") {
        CreateOverlayScene();
        return true;
    }
#else
    if (GetCurrentWorldName() == URL) return false;
    UnloadScene(GetCurrentWorldName());
    LoadScene(URL);
    return true;
#endif
}

bool Plu::SceneManager::IsAnySceneOpen()
{
    return GetCurrentWorld().IsValid();
}

void Plu::SceneManager::RegisterSceneInfo(TUsePointer<SceneInfo> sceneInfo)
{
    sceneInfo->URL.Replace(" ","");
    if (sceneInfo->URL == "Overlay" || sceneInfo->URL == "ActiveScene") {
        return;
    }
    mRegisteredScenesByURL[sceneInfo->URL] = sceneInfo;
}

Plu::TUsePointer<Plu::SceneWorld> Plu::GetCurrentWorld()
{
    return gSceneManager ? gSceneManager->GetCurrentWorld() : nullptr;
}
