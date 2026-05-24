//
// Created by Plutex on 2026-01-24.
//

#include "EditorPythonManager.h"

#include "EditorAppContext.h"
#pragma warning(push, 0)
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#pragma warning(pop)
#include "Python.h"
#include "Managers/Project/EditorProjectManager.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

Plu::EditorPythonManager::EditorPythonManager()
{
	if (!Py_IsInitialized()) {
		Py_Initialize();
	}
	pybind11::module_ main_module = pybind11::module_::import("__main__");
}

Plu::EditorPythonManager::~EditorPythonManager()
{
	Py_Finalize();
}

void Plu::EditorPythonManager::RunProjectScripts()
{
	for (const auto& script : std::filesystem::recursive_directory_iterator(gEditorAppContext->EditorProjectManager->GetProjectScriptsDirectory().CStr())) {
		PathW scriptPath = script.path().wstring().c_str();
		if (scriptPath.GetStem() == gEditorAppContext->EditorProjectManager->GetProjectName()) continue;
		if (scriptPath.GetExtension() != L".py") continue;
		RunScript(scriptPath, gEditorAppContext->EditorProjectManager->GetProjectScriptsDirectory(), "");
	}
}

void Plu::EditorPythonManager::ClearProjectScripts()
{
	pybind11::dict modules = pybind11::module_::import("sys").attr("modules");
	for (const auto& name : mUserModules) {
		//TODO reload
		if (modules.contains(name.CStr())) {
			modules.attr("pop")(name.CStr());
		}
	}
	mUserModules.Clear();
}

bool Plu::EditorPythonManager::RunScript(PluUUID uuid)
{
	return false;
}

bool Plu::EditorPythonManager::RunScript(PathW path, PathW workDir, String args)
{
	try {
		// 1. Ustawienie katalogu roboczego
		PathW orging = std::filesystem::current_path().wstring().c_str();
		std::filesystem::current_path(workDir.CStr());

		pybind11::module_ sys = pybind11::module_::import("sys");

		// 1. sys.path
		pybind11::list path_list = sys.attr("path");
		path_list.insert(0, path.GetParentPath().CStr());

		// 2. sys.argv - upewniamy się, że to czysta lista stringów
		pybind11::list py_argv;
		py_argv.append(pybind11::str(path.ToString().ToNarrow().CStr()));

		std::stringstream ss(args.CStr());
		std::string arg;
		while (ss >> arg) {
			py_argv.append(pybind11::str(arg));
		}
		sys.attr("argv") = py_argv;

		// 3. Przekierowanie stdout/stderr bez operatora _a
		pybind11::module_ builtins = pybind11::module_::import("builtins");
		builtins.attr("print_to_plu") = pybind11::cpp_function([this](std::string m, bool isError) {
			if (isError) PLU_ERROR("[Python] {}", m);
			else PLU_INFO("[Python] {}", m);
		});

		pybind11::exec(R"(
import sys
import builtins

class LogRedirector:
    def __init__(self, callback):
        self.callback = callback
    def write(self, m):
        msg = m.strip()
        if msg: self.callback(msg)
    def flush(self): pass

# Pobieramy funkcję bezpośrednio z builtins, żeby uniknąć problemów z zasięgiem
sys.stdout = LogRedirector(lambda msg: builtins.print_to_plu(msg, False))
sys.stderr = LogRedirector(lambda msg: builtins.print_to_plu(msg, True))
)");

		// 4. Uruchomienie skryptu
		std::string moduleName = path.GetStem().ToNarrow().CStr();

		// Usuń stary moduł z cache jeśli istnieje
		pybind11::dict modules = sys.attr("modules");
		if (modules.contains(moduleName)) {
			modules.attr("pop")(moduleName);
		}

		pybind11::module_::import(moduleName.c_str());
		PLU_TRACE("Run Script {}", moduleName.c_str());

		// Dodaj do listy tylko jeśli jeszcze nie ma
		if (!mUserModules.Contains(path.GetStem().ToNarrow())) {
			mUserModules.PushBack(path.GetStem().ToNarrow());
		}

		return true;
	} catch (const pybind11::error_already_set& e) {
		PLU_ERROR("Python Error: {}", e.what());
		return false;
	} catch (const std::exception& e) {
		PLU_ERROR("System Error: {}", e.what());
		return false;
	}
}
