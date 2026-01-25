//
// Created by Plutex on 2026-01-24.
//

#include "EditorPythonManager.h"
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "Python.h"

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

bool Plu::EditorPythonManager::RunScript(PluUUID uuid)
{
	return false;
}

bool Plu::EditorPythonManager::RunScript(PathW path, PathW workDir, String args)
{
	try {
		std::filesystem::current_path(workDir.CStr());

		// 2. Dodanie folderu ze skryptem do sys.path (żeby importy działały)
		pybind11::module_ sys = pybind11::module_::import("sys");
		sys.attr("path").cast<pybind11::list>().insert(0, path.GetParentPath().CStr());

		// 3. Przekazanie argumentów (symulacja sys.argv)
		// Python oczekuje, że sys.argv[0] to nazwa skryptu
		std::vector<std::string> argv_list = { path.GetStem().ToNarrow().CStr() };
		argv_list.push_back(args.CStr()); // Tu możesz też rozbić string na listę słów
		sys.attr("argv") = argv_list;

		pybind11::eval_file(path.ToString().ToNarrow().CStr());
		return true;
	} catch (const pybind11::error_already_set& e) {
		PLU_ERROR("Python Error: {}", e.what());
		return false;
	} catch (const std::exception& e) {
		PLU_ERROR("System Error: {}", e.what());
		return false;
	}
}
