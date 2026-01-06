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

		newDirectory += StringW::FromNarrow(name.CStr()).CStr();
		newDirectory += L"/";
		newDirectory += PLU_PROJECT_EXT_W;
		mCurrentProjectPath = newDirectory;
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
	}
}
