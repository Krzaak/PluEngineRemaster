//
// Created by Plutex on 1/20/26.
//

#ifndef PLUENGINE_EDITORSHADERCODE_H
#define PLUENGINE_EDITORSHADERCODE_H
#include "PluEngine/Core.h"
#include "PluEngine/Shaders/ShaderCode.h"
#include "EditorShaderCode.generated.h"

namespace Plu
{
	PLU_CLASS()
	class EditorShaderCode : public IShaderCode
	{
		REFLECTION_BODY_EDITORSHADERCODE()
	private:
		PLU_PROPERTY()
		PathW mPath;

		DynamicArray<TOwningPointer<IShaderUniform>> mUniforms;
	public:
		EditorShaderCode() = default;
		~EditorShaderCode() override;

		PathW GetPath();
		void Init(const PathW &path);
		String GetCode() override;
		DynamicArray<TOwningPointer<IShaderUniform>>* GetCodeUniforms() override;
		void RenewUniforms() override;
	};
}

#endif //PLUENGINE_EDITORSHADERCODE_H