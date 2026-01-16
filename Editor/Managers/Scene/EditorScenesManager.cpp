//
// Created by Plutex on 1/11/26.
//

#include "EditorScenesManager.h"

#include "json.hpp"
#include "json_fwd.hpp"
#include "detail/meta/std_fs.hpp"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"

bool Plu::EditorScenesManager::OpenSceneInternal(String url)
{
}

Plu::EditorScenesManager::EditorScenesManager()
{
}

Plu::EditorScenesManager::~EditorScenesManager()
{
}

void Plu::EditorScenesManager::CreateNewScene(String name, PathW path)
{
	path += L"/";
	path += name.ToWide();
	path += PLU_SCENE_EXT_W;

	nlohmann::json json = {
		{"uuid", PluUUID().getUUID()},
		{"type", "Scene"}
	};
	json["gameObjects"] = nlohmann::json::array();
	DiskManager::SaveJson(path.ToString(), json);
}

void Plu::EditorScenesManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager,
                                    const TUsePointer<EngineObjectManager> &engineObjectManager)
{
	mEditorProjectManager = editorProjectManager;
	mEngineObjectManager = engineObjectManager;
}

bool Plu::EditorScenesManager::ConnectToWorld(String URL)
{
	if (mSceneURLs.Contains(URL)) {
		return OpenSceneInternal(URL);
	}
	return false;
}

Plu::String Plu::EditorScenesManager::GetCurrentWorldName()
{
}
