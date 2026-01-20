//
// Created by Plutex on 1/20/26.
//

#ifndef PLUENGINE_EDITORSHADERMANAGER_H
#define PLUENGINE_EDITORSHADERMANAGER_H
#include "PluEngine/Managers/ShadersManager.h"
#include "EditorShaderManager.generated.h"

namespace Plu
{
	class EditorProjectManager;
	class EditorShaderCode;
	PLU_CLASS()
	class EditorShaderManager : public IShaderManager
	{
		REFLECTION_BODY_EDITORSHADERMANAGER()
	private:
		GameHashMap<MaxUInt64, TOwningPointer<ShaderProgram>> mShaderPrograms;
		GameHashMap<MaxUInt64, TOwningPointer<EditorShaderCode>> mShaderCodes;

		TUsePointer<EditorProjectManager> mProjectManager;
	public:
		EditorShaderManager();
		~EditorShaderManager() override;

		void PreInit(TUsePointer<EditorProjectManager> editorProjectManager);
		void ShaderCodeScan();
		void InitShaders();

		TUsePointer<IShaderCode> GetShaderCode(PluUUID uuid) override;
		TUsePointer<ShaderProgram> GetShaderProgram(PluUUID uuid) override;

	};
}

#endif //PLUENGINE_EDITORSHADERMANAGER_H