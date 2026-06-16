//
// Created by Plutex on 31.12.2025.
//

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_LINUX

#include "../RuntimeApp.h"
#include <argparse/argparse.hpp>

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("PluEngine (Runtime)");
	program.parse_args(argc, argv);
	Plu::Application* application = new Plu::RuntimeApp();
	application->Run();
	delete application;
	return 67;
}
#endif