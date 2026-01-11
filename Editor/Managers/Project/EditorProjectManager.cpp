//
// Created by Plutex on 1/6/26.
//

#include "EditorProjectManager.h"

#include <utility>

#include "adl_serializer.hpp"
#include "EditorAppContext.h"
#include "json.hpp"
#include "DefinedPanels/Project/ContentBrowserPanel/ContentBrowserPanel.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "Panels/EditorPanelManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"

namespace Plu
{
	EditorProjectManager::EditorProjectManager()
	{
		mCurrentProjectPath = L"";
	}

	EditorProjectManager::~EditorProjectManager()
	{
	}

	void EditorProjectManager::SetEditorAppContext(EditorAppContext *appContext, ApplicationInfo* applicationInfo)
	{
		mEditorAppContext = appContext;
		mApplicationInfo = applicationInfo;
	}

	bool EditorProjectManager::IsAnyProjectOpen() const
	{
		if (mCurrentProjectPath != L"" && mCurrentProjectPath.GetExtension() == PLU_PROJECT_EXT_W) return true;
		return false;
	}

	PathW EditorProjectManager::GetProjectDirectory() const
	{
		if (!IsAnyProjectOpen()) return L"";
		return mCurrentProjectPath.GetParentPath().ToString() + L"/";
	}

	StringW EditorProjectManager::GetProjectName() const
	{
		if (!IsAnyProjectOpen()) return L"";
		return mCurrentProjectPath.GetStem();
	}

	PathW EditorProjectManager::GetProjectPath() const
	{
		return mCurrentProjectPath.ToString();
	}

	bool EditorProjectManager::CreateNewProject(PathW newDirectory, const String& name)
	{
		nlohmann::json json = {
			{"IDK", "IDK"}
		};

		newDirectory += L"/";
		newDirectory += StringW::FromNarrow(name.CStr()).CStr();
		newDirectory += L"/";
		newDirectory += StringW::FromNarrow(name.CStr()).CStr();
		newDirectory += PLU_PROJECT_EXT_W;
		PLU_TRACE("New project at: {}", String::FromWide(newDirectory.CStr()).CStr());
		std::filesystem::create_directory(newDirectory.GetStem().CStr());
		mCurrentProjectPath = newDirectory;
		EnsureProjectStructure(newDirectory.GetParentPath());
		if (DiskManager::SaveJson(newDirectory.ToString(), json)) {
			OpenProject(mCurrentProjectPath);
			return true;
		} else {
			mCurrentProjectPath = L"";
			return false;
		}
	}

	bool EditorProjectManager::OpenProject(PathW projectPath)
	{
		PLU_INFO("Opening project at: {} ", String::FromWide(projectPath.CStr()).CStr());
		mCurrentProjectPath = std::move(projectPath);
		//Thats bad, I need to make an event system :(
		mEditorAppContext->EditorAssetManager->Init(mEditorAppContext->EditorProjectManager, mApplicationInfo->AppObjectManager);
		mEditorAppContext->EditorScenesManager->Init(mEditorAppContext->EditorProjectManager, mApplicationInfo->AppObjectManager);
		mEditorAppContext->EditorPanelManager->AddPanel(ContentBrowserPanel::GetStaticClass());
		return true;
	}

	void EditorProjectManager::EnsureProjectStructure(const PathW& projectPath)
	{
		std::filesystem::create_directory((projectPath.ToString() + L"/" + L"Assets").CStr());
		std::filesystem::create_directory((projectPath.ToString() + L"/" + L"Scripts").CStr());
		std::filesystem::create_directory((projectPath.ToString() + L"/" + L"Shaders").CStr());
		std::filesystem::create_directory((projectPath.ToString() + L"/" + L"Cache").CStr());
		std::filesystem::create_directory((projectPath.ToString() + L"/" + L"Config").CStr());
	}

	PathW EditorProjectManager::GetProjectConfigDirectory() const
	{
		return GetProjectDirectory().ToString() + L"Config";
	}

	PathW EditorProjectManager::GetProjectAssetsDirectory() const
	{
		return GetProjectDirectory().ToString() + L"Assets";
	}

	PathW EditorProjectManager::GetProjectScriptsDirectory() const
	{
		return GetProjectDirectory().ToString() + L"Scripts";
	}

	PathW EditorProjectManager::GetProjectShadersDirectory() const
	{
		return GetProjectDirectory().ToString() + L"Shaders";
	}
}
