//
// Created by Plutex on 5/27/26.
//

#include "RuntimeApp.h"

#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Window/WindowManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Input/InputManager.h"

Plu::RuntimeApp::RuntimeApp()
{
}

Plu::RuntimeApp::~RuntimeApp()
{
}

void Plu::RuntimeApp::OnImGuiRender()
{
}

void Plu::RuntimeApp::OnInit()
{
    PLU_INFO("Runtime Init");
    WindowProperties windowProperties;
    windowProperties.Title = "Runtime Window";
    mApplicationInfo.AppWindowsManager->AddWindow(windowProperties);
    const EngineObjectHandle rendererHandle = mObjectManager->CreateObject<Renderer>();
    mApplicationInfo.AppRenderer = mObjectManager->GetObjectAsOwner<Renderer>(rendererHandle);
    mApplicationInfo.AppRenderer->Init(this);
    EngineObjectHandle inputManagerHandle = mObjectManager->CreateObject<InputManager>();
    mApplicationInfo.AppInputManager = mObjectManager->GetObjectAsUser<InputManager>(inputManagerHandle);
}

void Plu::RuntimeApp::OnPostInit()
{
}

void Plu::RuntimeApp::OnShutdown()
{
}

void Plu::RuntimeApp::OnTick(float deltaTime)
{
}
