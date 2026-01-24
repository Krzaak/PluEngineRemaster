//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERPROGRAM_H
#define PLUENGINE_SHADERPROGRAM_H
#include "glad.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderProgram.generated.h"
#include <algorithm>
#include <string>
#include <sstream>

#include "PluEngine/PluUUID.h"
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
	inline String Sanitize(const String& s)
	{
		std::string out = s.CStr();
		std::replace(out.begin(), out.end(), ' ', '_');
		std::replace(out.begin(), out.end(), '/', '_');
		return out.c_str();
	}

	inline String BuildShaderCacheName()
	{
		const char* vendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		const char* version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));

#ifdef PLU_PLATFORM_WINDOWS
		String system = "Windows";
#elif defined(PLU_PLATFORM_LINUX)
		String system = "Linux";
#elif defined(PLU_PLATFORM_MACOS)
		String system = "macOS";
#else
		String system = "UnknownOS";
#endif

		std::stringstream ss;
		ss << system.CStr() << "_"
		   << Sanitize(vendor).CStr() << "_"
		   << Sanitize(renderer).CStr() << "_"
		   << "Driver" << Sanitize(version).CStr();

		return ss.str().c_str();
	}

	PLU_STRUCT()
	struct PLU_API ShaderProgramInfo : IAssetInfo
	{
		REFLECTION_BODY_SHADERPROGRAMINFO()

		PLU_PROPERTY(UuidFor=IShaderCode)
		PluUUID VertexShaderUuid;

		PLU_PROPERTY(UuidFor=IShaderCode)
		PluUUID FragmentShaderUuid;
	};


	class IShaderCode;
	PLU_CLASS()
	class PLU_API ShaderProgram : public EngineObject
	{
		REFLECTION_BODY_SHADERPROGRAM()
	private:
		UInt16 mProgramID;

		//Elements
		TUsePointer<IShaderCode> mVertexShader;
		TUsePointer<IShaderCode> mFragmentShader;

		void SaveBinary();
	public:
		ShaderProgram();
		~ShaderProgram() override;

		[[nodiscard]] bool HasNecessarySubshaders() const;

		PluUUID Uuid;

		void SetVertexShader(TUsePointer<IShaderCode> vertexShader);
		void SetFragmentShader(TUsePointer<IShaderCode> fragmentShader);

		bool Recompile(); //Just straight up recompile the shader, no checks
		void LoadFromBinary(PathW inputPath); //Tries to load from binary, if fails then Recompiles
	};
}

#endif //PLUENGINE_SHADERPROGRAM_H
