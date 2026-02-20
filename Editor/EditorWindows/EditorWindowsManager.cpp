//
// Created by Plutex on 2026-02-20.
//

#include "EditorWindowsManager.h"

#include "EditorAppContext.h"
#include "PluEngine/Window/Window.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

Plu::EditorWindowsManager::~EditorWindowsManager()
{
}

void Plu::EditorWindowsManager::OnUpdate(float deltaTime)
{
}

void Plu::EditorWindowsManager::OnSwapBuffers()
{
}

void Plu::EditorWindowsManager::NewWindow()
{
	WindowProperties properties{};
	properties.Title = "New Window";
	TOwningPointer<IWindow> window = IWindow::PlutexCreateWindow(properties, gEngineObjectManager, gApplicationInfo);
	window->Init();
	mWindows.PushBack(window);
}
