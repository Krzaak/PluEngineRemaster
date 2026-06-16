//
// Created by Plutex on 6/7/26.
//

#include "RuntimePythonRunner.h"

#include "pybind11/eval.h"

Plu::RuntimePythonRunner::RuntimePythonRunner()
{
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    pybind11::module_ main_module = pybind11::module_::import("__main__");
}

Plu::RuntimePythonRunner::~RuntimePythonRunner()
{
    Py_Finalize();
}

bool Plu::RuntimePythonRunner::RunScript(PluUUID uuid)
{
	return false;
}

bool Plu::RuntimePythonRunner::RunScript(PathW path, PathW workDir, String args)
{
	RunScript(path.ToString().ToNarrow());
	return true;
}

void Plu::RuntimePythonRunner::RunScript(Path scriptPath)
{
    try {
		// 1. Ustawienie katalogu roboczego
		PathW orging = std::filesystem::current_path().wstring().c_str();

		pybind11::module_ sys = pybind11::module_::import("sys");

		// 1. sys.path
		pybind11::list path_list = sys.attr("path");
		path_list.insert(0, scriptPath.GetParentPath().CStr());

		// 2. sys.argv - upewniamy się, że to czysta lista stringów
		pybind11::list py_argv;
		py_argv.append(pybind11::str(scriptPath.ToString().CStr()));

		std::stringstream ss("");
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
		std::string moduleName = scriptPath.GetStem().CStr();

		// Usuń stary moduł z cache jeśli istnieje
		pybind11::dict modules = sys.attr("modules");
		if (modules.contains(moduleName)) {
			modules.attr("pop")(moduleName);
		}

		pybind11::module_::import(moduleName.c_str());
		PLU_TRACE("Run Script {}", moduleName.c_str());
	} catch (const pybind11::error_already_set& e) {
		PLU_ERROR("Python Error: {}", e.what());
	} catch (const std::exception& e) {
		PLU_ERROR("System Error: {}", e.what());
	}
}

void Plu::RuntimePythonRunner::RunScripts(Path dir)
{
	for (const auto& script : std::filesystem::recursive_directory_iterator(dir.CStr())) {
		PathW scriptPath = script.path().wstring().c_str();
		if (scriptPath.GetExtension() != L".py") continue;
		RunScript(scriptPath.ToString().ToNarrow());
	}
}
