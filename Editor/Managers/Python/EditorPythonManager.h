//
// Created by Plutex on 2026-01-24.
//

#ifndef PLUENGINE_EDITORPYTHONMANAGER_H
#define PLUENGINE_EDITORPYTHONMANAGER_H
#include "PluEngine/Core.h"
#include "EditorPythonManager.generated.h"
#include "PluEngine/Managers/PythonManager.h"
#include "efsw/efsw.hpp"

namespace Plu
{
	class EFSWScriptsUpdateListener : public efsw::FileWatchListener
	{
	public:
		void handleFileAction(efsw::WatchID watchid, const std::string &dir, const std::string &filename, efsw::Action action, std::string oldFilename) override;
	};

	PLU_CLASS()
	class EditorPythonManager final : public IPythonManager
	{
		REFLECTION_BODY_EDITORPYTHONMANAGER()
	private:
		DynamicArray<String> mUserModules;
		HashSet<Path> mModulesLoaded;

		efsw::WatchID mProjectScriptsWatchId;
		efsw::FileWatcher* mFileWatcher = nullptr;
		EFSWScriptsUpdateListener* mListener = nullptr;
	public:
		EditorPythonManager();
		~EditorPythonManager() override;

		void RunProjectScripts();
		void ClearProjectScripts();

		void InitializeScriptsWatcher();
		void CheckForScriptsChanges();

		bool RunScript(PluUUID uuid) override;
		bool RunScript(PathW path, PathW workDir, String args) override;
	};
}

#endif //PLUENGINE_EDITORPYTHONMANAGER_H