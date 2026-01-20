//
// Created by Plutex on 1/19/26.
//

#include "glad/glad.h"

#include "PluEngine/Shaders/ShaderProgram.h"

#include "PluEngine/PluPaths.h"
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

bool Plu::ShaderProgram::Recompile()
{
	PLU_CORE_ASSERT(HasNecessarySubshaders(), "Triggered Recompile on Unready Shader!")

	MaxUInt16 vs = glCreateShader(GL_VERTEX_SHADER);
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

	return true;
}

void Plu::ShaderProgram::LoadFromBinary(PathW inputPath)
{
}
