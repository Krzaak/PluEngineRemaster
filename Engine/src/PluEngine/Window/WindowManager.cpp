//
// Created by Plutex on 2026-02-28.
//
#include "PluEngine/Window/WindowManager.h"

#include "PluEngine/Window/Window.h"

Plu::WindowsManager::WindowsManager() : mApplicationInfo(nullptr)
{
}

Plu::WindowsManager::~WindowsManager()
{
}

void Plu::WindowsManager::Init(const TUsePointer<EngineObjectManager> &objectManager, ApplicationInfo *applicationInfo)
{
	mApplicationInfo = applicationInfo;
	mEngineObjectManager = objectManager;
}

void Plu::WindowsManager::AddWindow(const WindowProperties &windowProperties)
{
	TOwningPointer<IWindow> newWindow = IWindow::PlutexCreateWindow(windowProperties, mEngineObjectManager, mApplicationInfo);
	newWindow->Init();
	mWindows.PushBack(newWindow);
}

void Plu::WindowsManager::UpdateEvents() const
{
	for (auto window : mWindows)
	{
		window->OnUpdate(0);
	}
}

Plu::TUsePointer<Plu::IWindow> Plu::WindowsManager::GetFirstWindow()
{
	return mWindows[0];
}

UInt64 Plu::WindowsManager::GetWindowsAmount() const
{
	return mWindows.Size();
}

Plu::TUsePointer<Plu::IWindow> Plu::WindowsManager::GetWindowAt(UInt64 index)
{
	return index < mWindows.Size() ? mWindows[index] : nullptr;
}
