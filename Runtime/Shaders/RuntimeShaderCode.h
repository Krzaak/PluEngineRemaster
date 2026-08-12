//
// Created by Plutex on 6/5/26.
//

#ifndef PLUENGINE_RUNTIMESHADERCODE_H
#define PLUENGINE_RUNTIMESHADERCODE_H

#include "PluEngine/AssetTypes/ShaderCode.h"
#include "RuntimeShaderCode.generated.h"

namespace Plu
{
    PLU_CLASS()
    class RuntimeShaderCode : public IShaderCode
    {
        REFLECTION_BODY_RUNTIMESHADERCODE()
    private:
        String mCode;
    public:
        RuntimeShaderCode() = default;
        explicit RuntimeShaderCode(String lineFromArchive);
        virtual ~RuntimeShaderCode() override = default;

        String GetCode() override;
        DynamicArray<TOwningPointer<IShaderUniform>> *GetCodeUniforms() override;
        void RenewUniforms() override;
    };
}



#endif //PLUENGINE_RUNTIMESHADERCODE_H
