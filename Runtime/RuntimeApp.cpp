//
// Created by Plutex on 5/27/26.
//

#include "RuntimeApp.h"

#include "PluEngine/PluGame.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Window/WindowManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "Python/RuntimePythonRunner.h"
#include "Shaders/RuntimeShaderManager.h"

Plu::RuntimeApp::RuntimeApp()
{
}

Plu::RuntimeApp::~RuntimeApp()
{
}

void Plu::RuntimeApp::OnImGuiRender()
{
}

bool Plu::RuntimeApp::OnInit()
{
    PLU_INFO("Runtime Init");
    WindowProperties windowProperties;
    PathW selfPath = GetExePath();

    PLU_INFO("Path is {}", selfPath.ToString().ToNarrow().CStr());

    PathW projectPath = selfPath.GetParentPath();
    projectPath /= L"ProjectDefaults.json";
    if (!std::filesystem::exists(projectPath.CStr())) {
        return false;
    }
    StringW exeName = selfPath.GetStem();
    windowProperties.Title = exeName.ToNarrow();
    mApplicationInfo.AppWindowsManager->AddWindow(windowProperties);
    const EngineObjectHandle rendererHandle = mObjectManager->CreateObject<Renderer>();
    mApplicationInfo.AppRenderer = mObjectManager->GetObjectAsOwner<Renderer>(rendererHandle);
    mApplicationInfo.AppRenderer->Init(this);
    EngineObjectHandle inputManagerHandle = mObjectManager->CreateObject<InputManager>();
    mApplicationInfo.AppInputManager = mObjectManager->GetObjectAsUser<InputManager>(inputManagerHandle);

    EngineObjectHandle shaderManagerHandle = mObjectManager->CreateObject<RuntimeShaderManager>();
    TOwningPointer<RuntimeShaderManager> shaderManager = mObjectManager->GetObjectAsOwner<RuntimeShaderManager>(shaderManagerHandle);
    shaderManager->ShaderCodeScan();
    mApplicationInfo.AppShaderManager = shaderManager;
    shaderManager->InitAssetEvents(mApplicationInfo.AppAssetManager, mApplicationInfo.AppObjectManager);

    mApplicationInfo.AppAssetManager->ScanDirectory(selfPath.GetParentPath().ToString().ToNarrow());

    mApplicationInfo.AppPythonManager = mObjectManager->CreateObject(RuntimePythonRunner::GetStaticClass());
    DynamicCast<RuntimePythonRunner>(mApplicationInfo.AppPythonManager)->RunScripts(selfPath.GetParentPath().ToString().ToNarrow() + "/Scripts");
    return true;
}

void Plu::RuntimeApp::OnPostInit()
{
    PLU_INFO("Runtime Post Init");
    StartGame();
    mApplicationInfo.AppInputManager->GetInputBackend()->SetMouseCentered(true);
    PathW selfPath = GetExePath().GetParentPath();
    mGameStartupSettings = CreateOwning<GameStartupSettings>();
    selfPath += L"/ProjectDefaults.json";
    std::optional<JSON> json = DiskManager::LoadJson(selfPath);
    if (!json.has_value())
        return;
    mGameStartupSettings = CreateOwning<GameStartupSettings>();
    DeserializationContext* dc = mApplicationInfo.ConstructDeserializationContext();
    TypeSerializer<TypeInfo*>::Deserialize(dc, json, GameStartupSettings::GetStaticClass(), mGameStartupSettings.GetRaw());
    TUsePointer<SceneInfo> sceneToLoadUUID = mGameStartupSettings->GameStartupScene;
    mApplicationInfo.AppScenesManager->ConnectToWorld(sceneToLoadUUID->URL);
    if (mApplicationInfo.AppScenesManager->IsAnySceneOpen()) {
        mApplicationInfo.Client->JoinGameLocally();
    }
}

void Plu::RuntimeApp::OnShutdown()
{
    EndGame();
    mApplicationInfo.AppRenderer->SetCamera(nullptr);
    mObjectManager->DestroyObject(*mApplicationInfo.AppScenesManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mApplicationInfo.AppAssetManager->GetEngineObjectHandle());
}

void Plu::RuntimeApp::OnTick(float deltaTime)
{
    static bool lastFocus = false;
    bool currentFocus = mApplicationInfo.AppWindow->HasWindowFocus();
    if (currentFocus != lastFocus)
    {
        if (currentFocus)
        {
            mApplicationInfo.AppWindow->SetCursorVisibility(false);
        } else
        {
            mApplicationInfo.AppWindow->SetCursorVisibility(true);
        }
        lastFocus = currentFocus;
    }
    mApplicationInfo.AppRenderer->GetMainBuffer()->BlitToScreen(mApplicationInfo.AppWindow->GetWidth(), mApplicationInfo.AppWindow->GetHeight());
    //mApplicationInfo.AppWindow->SetCursorPosition(mApplicationInfo.AppWindow->GetCursorPosition() + IVec2(-mApplicationInfo.AppInputManager->GetInputBackend()->GetMouse().deltaX, -mApplicationInfo.AppInputManager->GetInputBackend()->GetMouse().deltaY));
}
