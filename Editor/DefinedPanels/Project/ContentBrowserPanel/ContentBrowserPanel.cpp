//
// Created by Plutex on 1/10/26.
//

#include "ContentBrowserPanel.h"

#include <filesystem>
#include <utility>

#include "EditorAppContext.h"
#include "ImGuiFileDialog.h"
#include "UI/IconsFontAwesome7.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluSTL_FWD.h"
#include "EditorViewports/EditorViewportManager.h"
#include "Managers/Assets/EditorAssetManager.h"

ImGuiTreeNodeFlags dirFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
ImGuiTreeNodeFlags fileFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

void Plu::ContentBrowserPanel::FileNode(const PathW& path)
{
	ImGuiTreeNodeFlags fileFlags2 = fileFlags;
	if (path == mSelectedFile) {
		fileFlags2 |= ImGuiTreeNodeFlags_Selected;
	}
	if (ImGui::TreeNodeEx(String::FromWide(path.GetStem().CStr()).CStr(), fileFlags2)) {
		if (ImGui::IsItemClicked() && mSelectedFile == path) {
			PLU_TRACE("File at: {}", String::FromWide(path.GetStem().CStr()).CStr());
			TUsePointer<IEditorAssetObject> asset = mEditorAppContext->EditorAssetManager->GetAssetByPath(path);
			PLU_ASSERT(asset, "Selected file is not an Asset!")
			TypeInfo* viewportClass = mEditorAppContext->EditorAssetManager->GetAssetViewportClass(asset);
			PLU_ASSERT(viewportClass, "Asset doesn't have a valid ViewportClass")
			mEditorAppContext->EditorViewportManager->CreateViewport(path, viewportClass);
		}

		if (ImGui::IsItemClicked()) {
			mSelectedFile = path;
		}
	}
}

void Plu::ContentBrowserPanel::DirectoryNode(const PathW& path)
{
	if (ImGui::TreeNodeEx(String::FromWide(path.GetFilename().CStr()).CStr(), dirFlags))
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path.ToString().CStr())) {
			if (entry.is_directory()) {
				DirectoryNode(entry.path().wstring().c_str());
			} else {
				FileNode(entry.path().wstring().c_str());
			}
		}
		ImGui::TreePop();
	}
}

void Plu::ContentBrowserPanel::EntryNode(const PathW& start)
{
	DirectoryNode(start);
}

void Plu::ContentBrowserPanel::OnUpdate(float deltaTime)
{
	if (ImGui::Begin(ICON_FA_FOLDER_OPEN " Project Content")) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
		if (ImGui::Button(ICON_FA_FILE_CIRCLE_PLUS))
		{
			ImGuiFileDialog::Instance()->OpenDialog(
				"ImportAsset",
				"Select Asset",
				".fbx, .obj, .png, .jpg",
				IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
			);
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLUS))
		{

		}
		ImGui::PopStyleColor();
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory());
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectScriptsDirectory());
		EntryNode(mEditorAppContext->EditorProjectManager->GetProjectShadersDirectory());
	}
	ImGui::End();
	if (ImGuiFileDialog::Instance()->Display("ImportAsset"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
			mEditorAppContext->EditorAssetManager->ImportAssets({PathW(StringW::FromNarrow(filePath.c_str()))}, mEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory());
		}

		ImGuiFileDialog::Instance()->Close();
	}
}

void Plu::ContentBrowserPanel::OnHide()
{
}

void Plu::ContentBrowserPanel::OnShow()
{
}
