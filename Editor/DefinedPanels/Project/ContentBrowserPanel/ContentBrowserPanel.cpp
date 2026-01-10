//
// Created by Plutex on 1/10/26.
//

#include "ContentBrowserPanel.h"

#include <filesystem>
#include <utility>

#include "EditorAppContext.h"
#include "UI/IconsFontAwesome7.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluSTL_FWD.h"

void Plu::ContentBrowserPanel::FileNode(PathW path)
{
	if (ImGui::TreeNode(String::FromWide(path.GetStem().CStr()).CStr())) {
		PLU_TRACE("File at: {}", String::FromWide(path.GetStem().CStr()).CStr());
	}
}

void Plu::ContentBrowserPanel::DirectoryNode(PathW path)
{
	for (auto entry : std::filesystem::directory_iterator(path.ToString().CStr())) {
		if (entry.is_directory()) {
			DirectoryNode(entry.path().wstring().c_str());
		} else {
			FileNode(entry.path().wstring().c_str());
		}
	}
}

void Plu::ContentBrowserPanel::EntryNode(PathW start)
{
	DirectoryNode(std::move(start));
}

void Plu::ContentBrowserPanel::OnUpdate(float deltaTime)
{
	if (ImGui::Begin(ICON_FA_FOLDER_OPEN " Project Content")) {
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory());
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectScriptsDirectory());
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectShadersDirectory());
	}
	ImGui::End();
}

void Plu::ContentBrowserPanel::OnHide()
{
}

void Plu::ContentBrowserPanel::OnShow()
{
}
