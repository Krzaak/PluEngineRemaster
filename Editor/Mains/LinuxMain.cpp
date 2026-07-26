//
// Created by Plutex on 31.12.2025.
//

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_LINUX

#include "../EditorApp.h"
#include <argparse/argparse.hpp>

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("PluEngine");
	program.add_argument("--project", "-p").help("Path to project (.pluproject file or its directory) to launch at startup");
	Plu::Application::AddEngineArguments(program);
	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}
	Plu::Application* application = new Plu::PluEditor();
	application->InjectArguments(&program);
	application->Run();
	delete application;
	return 0;
}
#endif