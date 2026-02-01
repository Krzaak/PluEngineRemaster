//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderCode.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluTypes.h"

extern Plu::EditorAppContext* gEditorAppContext;


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

	PathW uniformsPath = gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory();
	uniformsPath += L"/ShaderParse/" + mPath.GetFilename() + L".txt";

#ifdef PLU_PLATFORM_WINDOWS
	if (std::filesystem::exists(uniformsPath.CStr()))
	{
#error "Dodaj no tam tego openera z std:: SCHNELL!"
#else
	if (std::filesystem::exists(uniformsPath.ToString().ToNarrow().CStr()))
	{
		std::ifstream file(uniformsPath.ToString().ToNarrow().CStr());
#endif
		std::string line;
		while (std::getline(file, line)) {
			String linePlu = line.c_str();
			DynamicArray<String> info = linePlu.Split(' ');
			const String& name = info[1];
			const String& type = info[0];
			int arraySize = -1;
			if (info.Size() > 2) {
				arraySize = info[2].ToInt();
			}
			if (type == "int") {
				TOwningPointer intUniform = CreateOwning<ShaderUniform<int>>();
				mUniforms.PushBack(intUniform);
				mUniformsUse.PushBack(intUniform);
				intUniform->Name = name;
				intUniform->Type = type;
				intUniform->ArraySize = arraySize;
			} else if (type == "float") {
				TOwningPointer floatUniform = CreateOwning<ShaderUniform<float>>();
				mUniforms.PushBack(floatUniform);
				mUniformsUse.PushBack(floatUniform);
				floatUniform->Name = name;
				floatUniform->Type = type;
				floatUniform->ArraySize = arraySize;
			} else if (type == "vec3") {
				TOwningPointer vec3Uniform = CreateOwning<ShaderUniform<Vec3>>();
				mUniforms.PushBack(vec3Uniform);
				mUniformsUse.PushBack(vec3Uniform);
				vec3Uniform->Name = name;
				vec3Uniform->Type = type;
				vec3Uniform->ArraySize = arraySize;
			} else if (type == "vec2") {
				TOwningPointer vec2Uniform = CreateOwning<ShaderUniform<Vec2>>();
				mUniforms.PushBack(vec2Uniform);
				mUniformsUse.PushBack(vec2Uniform);
				vec2Uniform->Name = name;
				vec2Uniform->Type = type;
				vec2Uniform->ArraySize = arraySize;
			} else if (type == "vec4") {
				TOwningPointer vec4Uniform = CreateOwning<ShaderUniform<Vec4>>();
				mUniforms.PushBack(vec4Uniform);
				mUniformsUse.PushBack(vec4Uniform);
				vec4Uniform->Name = name;
				vec4Uniform->Type = type;
				vec4Uniform->ArraySize = arraySize;
			} else if (type == "bool") {
				TOwningPointer boolUniform = CreateOwning<ShaderUniform<bool>>();
				mUniforms.PushBack(boolUniform);
				mUniformsUse.PushBack(boolUniform);
				boolUniform->Name = name;
				boolUniform->Type = type;
				boolUniform->ArraySize = arraySize;
			}
		}

	} else {
		PLU_ERROR("Uniforms for shader {} don't exist in Cache!", Name.CStr());
	}
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

DynamicArray<Plu::TUsePointer<Plu::IShaderUniform>>* Plu::EditorShaderCode::GetCodeUniforms()
{
	return &mUniformsUse;
}
