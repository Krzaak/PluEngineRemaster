//
// Created by Plutex on 31.12.2025.
//

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_LINUX

#include "../EditorApp.h"

int main(int argc, char** argv)
{
	Plu::Application* application = new Plu::PluEditor();
	application->Run();
	delete application;
	return 67;
}
#endif