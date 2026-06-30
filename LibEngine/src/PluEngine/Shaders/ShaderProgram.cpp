//
// Created by Plutex on 1/19/26.
//

#include "glad/glad.h"

#include "PluEngine/Shaders/ShaderProgram.h"
#include "glm/gtc/type_ptr.hpp"

#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Renderer/GLTexture.h"
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
	out.close();
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

void Plu::ShaderProgram::RenderFromMaterial(MaterialInfo *materialInfo, TUsePointer<RenderingManager> renderingManager)
{
	// Startujemy od slotu zarezerwowanego przez silnik (np. mapy cieni kaskad zajmują 0..mSlotsUsed-1).
	int numOfTextures = mSlotsUsed;
	UInt32 numUniforms = materialInfo->MaterialParameters.Size();
	for (UInt32 i = 0; i < numUniforms; i++) {
		TUsePointer<IShaderUniform> uniform = materialInfo->MaterialParameters.At(i);
		if (!uniform) continue;
		if (uniform->ArraySize != 0) continue;
		// Uniform sterowany przez silnik (liczba kaskad cieni) — ustawiany globalnie w
		// Renderer::RenderSnapshot. Parser shaderów wciągnął go błędnie jako parametr materiału
		// (brak na liście engineOnlyUniforms), więc materiały mają zapisane cascadeCount=0; bez tego
		// pominięcia nadpisałyby tu globalne 4 zerem i wyłączyły próbkowanie cieni.
		if (uniform->Name == "cascadeCount") continue;
		if (uniform->Type == "int") {
			SetIntUniform(uniform->Name, static_cast<ShaderUniform<int>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "float") {
			SetFloatUniform(uniform->Name, static_cast<ShaderUniform<float>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "vec3") {
			SetVec3Uniform(uniform->Name, static_cast<ShaderUniform<Vec3>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "vec2") {
			SetVec2Uniform(uniform->Name, static_cast<ShaderUniform<Vec2>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "vec4") {
			SetVec4Uniform(uniform->Name, static_cast<ShaderUniform<Vec4>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "bool") {
			SetBoolUniform(uniform->Name, static_cast<ShaderUniform<bool>*>(uniform.GetRaw())->Data);
		} else if (uniform->Type == "sampler2D") {
			//PLU_CORE_ERROR("No Setter for type texture");
			ShaderUniform<TUsePointer<TextureInfo>>* textureUniform = static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(uniform.GetRaw());
			TUsePointer<Texture> texture = renderingManager->GetTextureForInfo(textureUniform->Data);
			if (!texture) {
				renderingManager->RequestTextureFromInfo(textureUniform->Data);
			} else {
				SetTextureUniform(uniform->Name, texture, numOfTextures);
				numOfTextures++;
			}
		}
	}
	numOfTextures = 0;
}

void Plu::ShaderProgram::SetMatrix4Uniform(String name, Matrix4 matrix)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniformMatrix4fv(mUniformLocationCache[name], 1, GL_FALSE, glm::value_ptr(matrix));
}

void Plu::ShaderProgram::SetVec2Uniform(String name, Vec2 vec)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform2fv(mUniformLocationCache[name], 1, glm::value_ptr(vec));
}

void Plu::ShaderProgram::SetVec3Uniform(String name, Vec3 vec)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform3fv(mUniformLocationCache[name], 1, glm::value_ptr(vec));
}

void Plu::ShaderProgram::SetVec4Uniform(String name, Vec4 vec)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform4fv(mUniformLocationCache[name], 1, glm::value_ptr(vec));
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

void Plu::ShaderProgram::SetBoolUniform(String name, bool value)
{
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	glUniform1i(mUniformLocationCache[name], value);
}

void Plu::ShaderProgram::SetTextureUniform(String name, TUsePointer<class Texture> texture, int textureUnit)
{
	if (!texture) return;
	Bind();
	if (!mUniformLocationCache.Contains(name)) {
		mUniformLocationCache[name] = glGetUniformLocation(mProgramID, name.CStr());
	}
	texture->Bind(textureUnit);
	glUniform1i(mUniformLocationCache[name], textureUnit);

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

	UnloadProgram();
	mProgramID = program;
	mUniformLocationCache.Clear();
	PLU_CORE_TRACE("Program recompiled OpenGL ID {}", mProgramID);
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
		Recompile();
		return;
	}
	mProgramID = program;
	PLU_CORE_INFO("Loaded program with UUID {} from binary with new ID {}", Uuid.getUUID(), mProgramID);
}
