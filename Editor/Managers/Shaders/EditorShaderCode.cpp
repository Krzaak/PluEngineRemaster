//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderCode.h"

Plu::PathW Plu::EditorShaderCode::GetPath()
{
	return mPath;
}

void Plu::EditorShaderCode::Init(const PathW &path)
{
	mPath = path;
	Name = mPath.GetStem().ToNarrow();
}

Plu::String Plu::EditorShaderCode::GetCode()
{
#ifdef PLU_PLATFORM_WINDOWS
	std::ifstream file(mPath);
#else
	std::ifstream file(mPath.ToString().ToNarrow().CStr());
#endif
	if (!file.is_open())
		throw std::runtime_error(("Failed to open shader file: " + mPath.ToString().ToNarrow()).CStr());

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str().c_str();
}
