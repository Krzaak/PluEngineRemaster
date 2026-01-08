//
// Created by Plutex on 1/6/26.
//

#include "EditorProjectManager.h"

#include "adl_serializer.hpp"
#include "json.hpp"
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

	bool EditorProjectManager::IsAnyProjectOpen() const
	{
		if (mCurrentProjectPath != L"" && mCurrentProjectPath.GetFileExtension() == PLU_PROJECT_EXT_W) return true;
		return false;
	}

	StringW EditorProjectManager::GetProjectDirectory() const
	{
		if (!IsAnyProjectOpen()) return L"";
		return mCurrentProjectPath.GetDirectory();
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

	bool EditorProjectManager::CreateNewProject(StringW newDirectory, String name)
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
			return true;
		} else {
			mCurrentProjectPath = L"";
			return false;
		}
	}

	bool EditorProjectManager::OpenProject(StringW projectPath)
	{
	}

	void EditorProjectManager::EnsureProjectStructure(StringW projectPath)
	{
		std::filesystem::create_directory((projectPath + L"/" + L"Assets").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Scripts").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Shaders").CStr());
		std::filesystem::create_directory((projectPath + L"/" + L"Cache").CStr());
	}
}
