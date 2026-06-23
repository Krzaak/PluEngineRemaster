//
// Created by Plutex on 6/22/26.
//

#include "PluEngine/Renderer/Renderer.h"

#include "PluEngine/Application.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"

void Plu::Renderer::Initialize(ApplicationInfo *applicationInfo)
{
    mApplicationInfo = applicationInfo;
    EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mMainBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->Create(window->GetWidth(), window->GetHeight(), mApplicationInfo->AppObjectManager, FrameBufferType::ColorDepth);
}

void Plu::Renderer::RenderSnapshot(Plu::RenderSnapshot *snapshot)
{
    mMainBuffer->Clear(1.0f);

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->BlitToScreen(window->GetWidth(), window->GetHeight());
}

void Plu::Renderer::Shutdown()
{
    mApplicationInfo->AppObjectManager->DestroyObject(mMainBuffer->GetObjectHandle());
    mMainBuffer->Destroy();
    mMainBuffer = nullptr;
}
