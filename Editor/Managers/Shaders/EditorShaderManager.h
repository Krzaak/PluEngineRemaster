//
// Created by Plutex on 1/20/26.
//

#ifndef PLUENGINE_EDITORSHADERMANAGER_H
#define PLUENGINE_EDITORSHADERMANAGER_H
#include <thomasmonkman-filewatch/FileWatch.hpp>

#include "PluEngine/Managers/ShadersManager.h"
#include "EditorShaderManager.generated.h"
#include "PluEngine/Shaders/ShaderCacheWriter.h"

namespace Plu
{
	class EditorProjectManager;
	class EditorShaderCode;

	PLU_CLASS()
	class EditorShaderWriter : public IShaderCacheWriter
	{
		REFLECTION_BODY_EDITORSHADERWRITER()
	public:
		EditorShaderWriter() = default;
		virtual ~EditorShaderWriter() override = default;
		PathW GetShaderCacheDirectory() override;
	};

	PLU_CLASS()
	class EditorShaderManager : public IShaderManager
	{
		REFLECTION_BODY_EDITORSHADERMANAGER()
	private:
		GameHashMap<UInt64, TOwningPointer<ShaderProgram>> mShaderPrograms;
		DynamicArray<TUsePointer<ShaderProgram>> mInitializedShaderPrograms;
		GameHashMap<UInt64, TOwningPointer<EditorShaderCode>> mShaderCodes;

		TUsePointer<EditorProjectManager> mProjectManager;

		DynamicArray<filewatch::FileWatch<std::string>*> mUnixWatchers;
	public:
		EditorShaderManager();
		~EditorShaderManager() override;

		void PreInit(TUsePointer<EditorProjectManager> editorProjectManager);
		void ShaderCodeScan();
		void InitShaders();

		TUsePointer<IShaderCode> GetShaderCode(PluUUID uuid) override;
		TUsePointer<ShaderProgram> GetShaderProgram(PluUUID uuid) override;
		DynamicArray<TUsePointer<ShaderProgram>> *GetRenderableShaderPrograms() override;
		void LoadShader(PluUUID uuid) override;
	};
}

#endif //PLUENGINE_EDITORSHADERMANAGER_H