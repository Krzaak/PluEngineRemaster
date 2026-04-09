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
	program.add_argument("--project").help("Path to project to launch at startup");
	program.parse_args(argc, argv);
	Plu::Application* application = new Plu::PluEditor();
	application->Run();
	delete application;
	return 67;
}
#endif