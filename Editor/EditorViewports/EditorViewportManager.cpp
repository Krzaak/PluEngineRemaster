#include "EditorViewportManager.h"

#include <imgui/imgui_internal.h>

#include "EditorAppContext.h"
#include "EditorInterface.h"
#include "IEditorViewport.h"
#include "DefinedViewports/Scene/SceneViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::EditorViewportManager::EditorViewportManager()
{
    window_class = new ImGuiWindowClass();
    window_class->DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;
}

Plu::EditorViewportManager::~EditorViewportManager()
{
}

Plu::TUsePointer<Plu::IEditorPanel> Plu::EditorViewportManager::GetHoveredPanel()
{
    return mHoveredPanel;
}

void Plu::EditorViewportManager::CreateViewport(const PathW& assetPath, const TypeInfo* classOfViewport)
{
    if (!classOfViewport) return;
    if (classOfViewport == SceneViewport::GetStaticClass() && GetViewport(SceneViewport::GetStaticClass())) {
        TUsePointer<IEditorAssetObject> asset = gEditorAppContext->EditorAssetManager->GetAssetByPath(assetPath);
        GetViewport(SceneViewport::GetStaticClass())->Initialize(asset);
        return;
    }
    TUsePointer<IEditorAssetObject> asset = gEditorAppContext->EditorAssetManager->GetAssetByPath(assetPath);
    TOwningPointer<IEditorViewport> viewport = gEngineObjectManager->GetObjectAsOwner<IEditorViewport>(gEngineObjectManager->CreateObject(classOfViewport)->GetObjectHandle());
    viewport->Initialize(asset);
    viewports.PushBack(viewport);
    viewport->OnOpened();

    mWindowsToDock.PushBack(viewport->GetWindowTitle());
}

Plu::TUsePointer<Plu::IEditorViewport> Plu::EditorViewportManager::GetViewport(TypeInfo *viewportClass)
{
    for (auto viewport : viewports) {
        if (viewport->GetClass() == viewportClass) return viewport;
    }
    return nullptr;
}

void Plu::EditorViewportManager::Tick(float deltaTime)
{
    if (!mWindowsToDock.IsEmpty())
    {
        for (const String& windowTitle : mWindowsToDock)
        {
            ImGui::DockBuilderDockWindow(windowTitle.CStr(), gDockspaceId);
        }
        ImGui::DockBuilderFinish(gDockspaceId);
        mWindowsToDock.Clear();
    }
    for (const TOwningPointer<IEditorViewport>& viewport : viewports)
    {
        if (!viewport->IsOpen())
        {
            viewport->OnClosed();
            //TODO !^
            // mAppContext->ApplicationObjectManager->DestroyObject(*viewport->GetHandle());
            // std::_Erase_remove(viewports, viewport);
            return;
        }
        viewport->OnUpdate(deltaTime);
    }
}

void Plu::EditorViewportManager::Shutdown()
{
    for (const auto& viewport : viewports)
    {
        viewport->OnClosed();
        viewport->Shutdown();
    }
    for (TOwningPointer<IEditorViewport> viewport : viewports)
    {
        gEngineObjectManager->DestroyObject(*viewport->GetEngineObjectHandle());
        viewport = nullptr;
    }
    viewports.Clear();
}

ImGuiWindowClass* Plu::EditorViewportManager::GetViewportWindowClass() const
{
    return window_class;
}
