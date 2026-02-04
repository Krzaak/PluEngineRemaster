//
// Created by Plutex on 1/19/26.
//

#include "glad/glad.h"

#include "PluEngine/Shaders/ShaderProgram.h"
#include "glm/gtc/type_ptr.hpp"

#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Shaders/ShaderCacheWriter.h"
#include "PluEngine/Shaders/ShaderCode.h"

void Plu::ShaderProgram::SaveBinary()
{
	PLU_CORE_ASSERT(mProgramID != 0, "Tried to save uncompiled Shader!")
	GLint binaryLength = 0;
	glGetProgramiv(mProgramID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

	std::vector<GLubyte> binary(binaryLength);
	GLenum binaryFormat = 0;

	glGetProgramBinary(mProgramID, binaryLength, nullptr, &binaryFormat, binary.data());;

	PathW outPath = GetGlobalShaderCacheWriter()->GetShaderCacheDirectory();
	outPath += L"/" + BuildShaderCacheName().ToWide() + L"/";
	outPath += StringW::FromInt(Uuid.getUUID()) + PLU_BINARY_EXT_W;
#ifdef PLU_PLATFORM_WINDOWS
	std::ofstream out(outPath.CStr(), std::ios::binary);
#else
	std::ofstream out(outPath.ToString().ToNarrow().CStr(), std::ios::binary);
#endif

	out.write(reinterpret_cast<const char*>(&binaryFormat), sizeof(binaryFormat));
	out.write(reinterpret_cast<const char*>(binary.data()), static_cast<long long>(binary.size()));
}

Plu::ShaderProgram::ShaderProgram()
{
	mProgramID = 0;
}

Plu::ShaderProgram::~ShaderProgram()
{
}

bool Plu::ShaderProgram::HasNecessarySubshaders() const
{
	return mFragmentShader && mVertexShader;
}

void Plu::ShaderProgram::SetVertexShader(TUsePointer<IShaderCode> vertexShader)
{
	mVertexShader = vertexShader;
}

void Plu::ShaderProgram::SetFragmentShader(TUsePointer<IShaderCode> fragmentShader)
{
	mFragmentShader = fragmentShader;
}

Plu::TUsePointer<Plu::IShaderCode> Plu::ShaderProgram::GetVertexShader()
{
	return mVertexShader;
}

Plu::TUsePointer<Plu::IShaderCode> Plu::ShaderProgram::GetFragmentShader()
{
	return mFragmentShader;
}

void Plu::ShaderProgram::RenderFromMaterial(MaterialInfo *materialInfo)
{
	for (const auto& uniform : materialInfo->MaterialParameters) {
		if (uniform->ArraySize != 0) continue;
		if (uniform->Type == "int") {
			SetIntUniform(uniform->Name, static_cast<ShaderUniform<int>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "float") {
			SetFloatUniform(uniform->Name, static_cast<ShaderUniform<float>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "vec3") {
			SetVec3Uniform(uniform->Name, static_cast<ShaderUniform<Vec3>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "vec2") {
			PLU_CORE_ERROR("No Setter for type Vec2");
		} else if (uniform->Type == "vec4") {
			PLU_CORE_ERROR("No Setter for type Vec4");
		} else if (uniform->Type == "bool") {
			PLU_CORE_ERROR("No Setter for type bool");
		}
	}
}

void Plu::ShaderProgram::SetMatrix4Uniform(String name, Matrix4 matrix)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniformMatrix4fv(mUniformLocationCache[name], 1, GL_FALSE, glm::value_ptr(matrix));
}

void Plu::ShaderProgram::SetVec3Uniform(String name, Vec3 vec)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform3fv(mUniformLocationCache[name], 1, glm::value_ptr(vec));
}

void Plu::ShaderProgram::SetIntUniform(String name, int value)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform1i(mUniformLocationCache[name], value);
}

void Plu::ShaderProgram::SetFloatUniform(String name, float value)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform1f(mUniformLocationCache[name], value);
}


void Plu::ShaderProgram::Bind() const
{
	glUseProgram(mProgramID);
}

bool Plu::ShaderProgram::Recompile()
{
	if (!HasNecessarySubshaders()) {
		PLU_ERROR("Triggered Recompile on Unready Shader!");
		return false;
	}

	UInt16 vs = glCreateShader(GL_VERTEX_SHADER);
	String vsrc = mVertexShader->GetCode();
	const char* vsrc_c = vsrc.CStr();
	glShaderSource(vs, 1, &vsrc_c, nullptr);
	glCompileShader(vs);

	int success;
	char infoLog[512];
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vs, 512, nullptr, infoLog);
		PLU_CORE_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n{}", infoLog);
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	String fsrc = mFragmentShader->GetCode();
	const char* fsrc_c = fsrc.CStr();
	glShaderSource(fs, 1, &fsrc_c, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fs, 512, nullptr, infoLog);
		PLU_CORE_ERROR("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n{}", infoLog);
		return false;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		PLU_CORE_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED\n{}", infoLog);
		return false;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	mProgramID = program;
	SaveBinary();
	return true;
}

void Plu::ShaderProgram::UnloadProgram()
{
	glDeleteProgram(mProgramID);
	mProgramID = 0;
}

bool Plu::ShaderProgram::BinaryExists() const
{
	PathW outPath = GetGlobalShaderCacheWriter()->GetShaderCacheDirectory();
	outPath += L"/" + BuildShaderCacheName().ToWide() + L"/";
	outPath += StringW::FromInt(Uuid.getUUID()) + PLU_BINARY_EXT_W;
	std::filesystem::create_directories(outPath.GetParentPath().CStr());
	return std::filesystem::exists(outPath.CStr());
}

void Plu::ShaderProgram::LoadFromBinary()
{
	PathW cachePath = GetGlobalShaderCacheWriter()->GetShaderCacheDirectory();
	cachePath += L"/" + BuildShaderCacheName().ToWide() + L"/";
	cachePath += StringW::FromInt(Uuid.getUUID()) + PLU_BINARY_EXT_W;
	if (!std::filesystem::exists(cachePath.CStr())) {
		Recompile();
		return;
	}
#ifdef PLU_PLATFORM_WINDOWS
	std::ifstream in(cachePath.CStr(), std::ios::binary);
#else
	std::ifstream in(cachePath.ToString().ToNarrow().CStr(), std::ios::binary);
#endif

	if (!in.is_open()) return;

	GLenum binaryFormat;
	in.read(reinterpret_cast<char*>(&binaryFormat), sizeof(binaryFormat));

	std::vector<char> binary(std::filesystem::file_size(cachePath.CStr()) - sizeof(binaryFormat));
	in.read(binary.data(), static_cast<long long>(binary.size()));

	GLuint program = glCreateProgram();
	glProgramBinary(program, binaryFormat, binary.data(), static_cast<GLsizei>(binary.size()));

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glDeleteProgram(program);
		PLU_CORE_ERROR("ERROR::SHADER::PROGRAM::BINARY_FAILED");
		return; // fallback: trzeba skompilować z tekstu
	}
	mProgramID = program;
	PLU_CORE_INFO("Loaded program with UUID {} from binary with new ID {}", Uuid.getUUID(), mProgramID);
}
