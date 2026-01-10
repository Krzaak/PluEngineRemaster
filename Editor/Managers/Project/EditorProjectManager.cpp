//
// Created by Plutex on 1/6/26.
//

#include "EditorProjectManager.h"

#include <utility>

#include "adl_serializer.hpp"
#include "EditorAppContext.h"
#include "json.hpp"
#include "Managers/Assets/EditorAssetManager.h"
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

	void EditorProjectManager::SetEditorAppContext(EditorAppContext *appContext)
	{
		mEditorAppContext = appContext;
	}

	bool EditorProjectManager::IsAnyProjectOpen() const
	{
		if (mCurrentProjectPath != L"" && mCurrentProjectPath.GetFileExtension() == PLU_PROJECT_EXT_W) return true;
		return false;
	}

	StringW EditorProjectManager::GetProjectDirectory() const
	{
		if (!IsAnyProjectOpen()) return L"";
		return mCurrentProjectPath.GetDirectory() + L"/";
	}

	StringW EditorProjectManager::GetProjectName() const
	{
		if (!IsAnyProjectOpen()) return L"";
		return mCurrentProjectPath.GetFileName();
	}

	StringW EditorProjectManager::GetProjectPath()
	{
		return mCurrentProjectPath;
	}

	bool EditorProjectManager::CreateNewProject(StringW newDirectory, const String& name)
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
		std::filesystem::create_directory(newDirectory.GetDirectory().CStr());
		mCurrentProjectPath = newDirectory;
		EnsureProjectStructure(newDirectory.GetDirectory());
		if (DiskManager::SaveJson(newDirectory, json)) {
			OpenProject(mCurrentProjectPath);
			return true;
		} else {
			mCurrentProjectPath = L"";
			return false;
		}
	}

	bool EditorProjectManager::OpenProject(StringW projectPath)
	{
		PLU_INFO("Opening project at: {} ", String::FromWide(projectPath.CStr()).CStr());
		mCurrentProjectPath = std::move(projectPath);
		mEditorAppContext->EditorAssetManager->Init(mEditorAppContext->EditorProjectManager);
		return true;
	}

	void EditorProjectManager::EnsureProjectStructure(const StringW& projectPath)
	{
		std::filesystem::create_directory((projectPath + L"/" + L"Assets").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Scripts").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Shaders").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Cache").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Config").CStr());
	}

	StringW EditorProjectManager::GetProjectConfigDirectory() const
	{
		return GetProjectDirectory() + L"Config/";
	}

	StringW EditorProjectManager::GetProjectAssetsDirectory() const
	{
		return GetProjectDirectory() + L"Assets/";
	}
}
