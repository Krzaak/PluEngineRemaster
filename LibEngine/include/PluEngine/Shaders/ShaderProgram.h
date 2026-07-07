//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERPROGRAM_H
#define PLUENGINE_SHADERPROGRAM_H
#include "glad.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderProgram.generated.h"
#include <algorithm>
#include <atomic>
#include <string>
#include <sstream>

#include "PluEngine/PluTypes.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
	struct MaterialInfo;

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
	struct PLU_API ShaderProgramInfo : IAssetData
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
		UInt16 mProgramID = 0;

		//Elements
		TUsePointer<IShaderCode> mVertexShader;
		TUsePointer<IShaderCode> mFragmentShader;

		GameHashMap<String, int> mUniformLocationCache;

		// Liczba jednostek teksturujących zarezerwowanych przez silnik (np. mapy cieni kaskad)
		// zanim materiał zacznie bindować swoje tekstury. RenderFromMaterial startuje od tej wartości.
		int mSlotsUsed = 0;

		// Hot reload: main-thread ustawia flagę gdy źródło shadera się zmieniło, a wątek renderu
		// (gdzie żyje kontekst GL) konsumuje ją i wykonuje faktyczny Recompile(). Wywołania GL
		// nie mogą lecieć z wątku main.
		std::atomic<bool> mRecompileRequested{false};

		// Cache zapytania o blok SSBO "BoneMatrices" (vertex skinning); -1 = jeszcze nie
		// sprawdzone dla aktualnie zlinkowanego programu. Resetowane przy każdej zmianie mProgramID.
		int mHasBoneMatricesBlock = -1;

		void SaveBinary();
	public:
		ShaderProgram();
		~ShaderProgram() override;

		[[nodiscard]] bool HasNecessarySubshaders() const;

		PluUUID Uuid;

		void SetVertexShader(TUsePointer<IShaderCode> vertexShader);
		void SetFragmentShader(TUsePointer<IShaderCode> fragmentShader);

		TUsePointer<IShaderCode> GetVertexShader();
		TUsePointer<IShaderCode> GetFragmentShader();

		[[nodiscard]] bool IsLoaded() const
		{ return mProgramID != 0; }

		void RenderFromMaterial(MaterialInfo* materialInfo, TUsePointer<class RenderingManager> renderingManager);

		// Ustawia ile jednostek teksturujących zajmuje silnik (cienie) zanim zacznie materiał.
		void SetSlotsUsed(int slots) { mSlotsUsed = slots; }
		[[nodiscard]] int GetSlotsUsed() const { return mSlotsUsed; }

		//Setters
		void SetMatrix4Uniform(String name, Matrix4 matrix);
		void SetVec2Uniform(String name, Vec2 vec);
		void SetVec3Uniform(String name, Vec3 vec);
		void SetVec4Uniform(String name, Vec4 vec);
		void SetIntUniform(String name, int value);
		void SetFloatUniform(String name, float value);
		void SetBoolUniform(String name, bool value);
		void SetTextureUniform(String name, TUsePointer<class Texture> texture, int textureUnit);

		void Bind() const;

		bool Recompile(); //Just straight up recompile the shader, no checks
		void UnloadProgram();

		// Zgłasza chęć rekompilacji z dowolnego wątku (main). Faktyczny GL Recompile robi
		// wątek renderu przez ConsumeRecompileRequest() -> Recompile().
		void RequestRecompile() { mRecompileRequested.store(true, std::memory_order_release); }
		bool ConsumeRecompileRequest() { return mRecompileRequested.exchange(false, std::memory_order_acq_rel); }

		[[nodiscard]] bool BinaryExists() const;
		void LoadFromBinary(); //Tries to load from binary, if fails then Recompiles

		// Czy zlinkowany program deklaruje blok SSBO "BoneMatrices" (skinning skeletal meshy).
		// Wołać z wątku renderu po IsLoaded(); wynik GL query jest cache'owany per link.
		[[nodiscard]] bool HasBoneMatricesBlock();
	};
}

#endif //PLUENGINE_SHADERPROGRAM_H
