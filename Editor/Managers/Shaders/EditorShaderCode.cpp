//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderCode.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"

extern Plu::EditorAppContext* gEditorAppContext;

Plu::EditorShaderCode::~EditorShaderCode()
{
	mUniforms.Clear();
}

Plu::Path Plu::EditorShaderCode::GetUniformsPath() const
{
	PathW uniformsPath = gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory();
	uniformsPath += L"/ShaderParse/" + mPath.GetFilename() + L".txt";
	return uniformsPath.ToString().ToNarrow();
}

Plu::PathW Plu::EditorShaderCode::GetPath()
{
	return mPath;
}

void Plu::EditorShaderCode::Init(const PathW &path)
{
	mPath = path;
	String extDot = mPath.GetExtension().ToNarrow();
	extDot.Remove(0, 1);
	Name = mPath.GetStem().ToNarrow() + extDot;

	RenewUniforms();
}

Plu::String Plu::EditorShaderCode::GetCode()
{
#ifdef PLU_PLATFORM_WINDOWS
	std::ifstream file(mPath.CStr());
#else
	std::ifstream file(mPath.ToString().ToNarrow().CStr());
#endif
	if (!file.is_open())
		throw std::runtime_error(("Failed to open shader file: " + mPath.ToString().ToNarrow()).CStr());

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str().c_str();
}

DynamicArray<Plu::TOwningPointer<Plu::IShaderUniform>>* Plu::EditorShaderCode::GetCodeUniforms()
{
	return &mUniforms;
}

void Plu::EditorShaderCode::RenewUniforms()
{
	mUniforms.Clear();
	PathW uniformsPath = GetUniformsPath().ToString().ToWide();

#ifdef PLU_PLATFORM_WINDOWS
	if (std::filesystem::exists(uniformsPath.CStr()))
	{
		std::ifstream file(uniformsPath.CStr());
#else
	if (std::filesystem::exists(uniformsPath.ToString().ToNarrow().CStr()))
	{
		std::ifstream file(uniformsPath.ToString().ToNarrow().CStr());
#endif
		std::string line;
		while (std::getline(file, line)) {
			String linePlu = line.c_str();
			HandleLineUniforms(linePlu);
		}

	} else {
		PLU_ERROR("Uniforms for shader {} don't exist in Cache!", Name.CStr());
	}
}
