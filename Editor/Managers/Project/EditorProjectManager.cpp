//
// Created by Plutex on 1/6/26.
//

#include "EditorProjectManager.h"

#include <utility>
#include "EditorAppContext.h"
#include "json_fwd.hpp"
#include "DefinedPanels/ProjectLauncherPanel.h"
#include "DefinedPanels/Project/ContentBrowserPanel/ContentBrowserPanel.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "Managers/Shaders/EditorShaderManager.h"
#include "Panels/EditorPanelManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Window/Window.h"

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

	EditorAppContext * EditorProjectManager::GetAppContext()
	{
		return mEditorAppContext;
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

	PathW EditorProjectManager::GetRecentProjectsJSONPath()
	{
		const PathW exeDir = GetExePath().GetParentPath();
		PathW recentProjectJsonPath = exeDir / L"RecentProjects.json";
		return recentProjectJsonPath;
	}

	PathW EditorProjectManager::GetEngineAssetsPath()
	{
		const PathW exeDir = GetExePath().GetParentPath();
		PathW recentProjectJsonPath = exeDir / L"EngineAssets";
		return recentProjectJsonPath;
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
		std::filesystem::create_directory(newDirectory.GetParentPath().CStr());
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
		EnsureProjectStructure(projectPath.GetParentPath());
		mCurrentProjectPath = projectPath;
		//Thats bad, I need to make an event system :(
		mEditorAppContext->EditorShaderManager->ShaderCodeScan();
		mEditorAppContext->EditorAssetManager->Init(mEditorAppContext->EditorProjectManager, mApplicationInfo->AppObjectManager);
		mEditorAppContext->EditorScenesManager->Init(mEditorAppContext->EditorProjectManager, mApplicationInfo->AppObjectManager);
		mEditorAppContext->EditorPanelManager->AddPanel(ContentBrowserPanel::GetStaticClass());

		//Recent Projects
		auto recentProjectsJson = DiskManager::LoadJson(GetRecentProjectsJSONPath());
		if (recentProjectsJson.has_value()) {
			nlohmann::json json = recentProjectsJson.value();
			bool has = false;
			for (const auto& project : json["projects"]) {
				if (project == projectPath.CStr()) {
					has = true;
					break;
				}
			}
			if (!has) {
				json["projects"].push_back(projectPath.CStr());
			}
			recentProjectsJson = json;
		} else {
			nlohmann::json json;
			json["projects"] = nlohmann::json::array();
			json["projects"].push_back(projectPath.CStr());
			recentProjectsJson = json;
		}
		DiskManager::SaveJson(GetRecentProjectsJSONPath().ToString(), recentProjectsJson.value());
		mEditorAppContext->EditorPanelManager->ClosePanel(*mEditorAppContext->EditorPanelManager->GetPanelByClass(TClassPointer<EditorPanel>(ProjectLauncherPanel::GetStaticClass()))->GetEngineObjectHandle());
		mApplicationInfo->AppWindow->SetWindowTitle(GetProjectName().ToNarrow());
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

	PathW EditorProjectManager::GetProjectCacheDirectory() const
	{
		return GetProjectDirectory().ToString() + L"Cache";
	}
}
