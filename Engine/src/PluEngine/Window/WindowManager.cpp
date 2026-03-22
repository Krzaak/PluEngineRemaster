//
// Created by Plutex on 2026-02-28.
//
#include "PluEngine/Window/WindowManager.h"

#include "PluEngine/Objects/EngineObjectManager.h"
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
	mWindowsToAdd.PushBack(windowProperties);
}

void Plu::WindowsManager::UpdateEvents() const
{
	for (auto window : mWindows)
	{
		if (!window) continue;
		window->OnUpdate(0);
	}
}

void Plu::WindowsManager::ProcessNewWindows()
{
	for (auto id : mWindowsToRemove) {
		GetWindowAt(id)->Close();
		mEngineObjectManager->DestroyObject(*GetWindowAt(id)->GetEngineObjectHandle());
		mWindows[id] = nullptr;
	}
	mWindowsToRemove.Clear();
	for (auto props : mWindowsToAdd) {
		TOwningPointer<IWindow> newWindow = IWindow::PlutexCreateWindow(props, mEngineObjectManager, mApplicationInfo);
		newWindow->Init();
		mWindows.PushBack(newWindow);
	}
	mWindowsToAdd.Clear();
}

void Plu::WindowsManager::CloseWindow(int windowID)
{
	mWindowsToRemove.PushBack(windowID);
	int wdID = windowID;
	DispatchEvent("ClosedWindow", &wdID);
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
