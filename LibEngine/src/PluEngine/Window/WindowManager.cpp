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
		TUsePointer<IWindow> win = GetWindowAt(id);
		if (!win) continue;
		win->Close();
		// The render thread renders & swaps the main AppWindow every frame. Destroying it here
		// (mid main-loop, via the editor's custom title-bar X -> CloseWindow) races the render
		// thread, which then calls SwapBuffer() on an already destroyed window (mWindow == null)
		// -> segfault inside SDL. Closing the main window means the app is exiting: Close() above
		// drops IsRunning() so Run() leaves the loop, and the normal shutdown path joins the render
		// thread BEFORE the window is destroyed (in EngineShutdown). Only secondary windows, which
		// the render thread never swaps, are torn down in-loop here.
		if (mApplicationInfo && win.GetRaw() == mApplicationInfo->AppWindow.GetRaw()) {
			continue;
		}
		mEngineObjectManager->DestroyObject(*win->GetEngineObjectHandle());
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
	if (mWindows.IsEmpty()) {
		return nullptr;
	}
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
