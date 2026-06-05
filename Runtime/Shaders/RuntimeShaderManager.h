//
// Created by Plutex on 6/5/26.
//

#ifndef PLUENGINE_RUNTIMESHADERMANAGER_H
#define PLUENGINE_RUNTIMESHADERMANAGER_H

#include "PluEngine/Managers/ShadersManager.h"
#include "RuntimeShaderManager.generated.h"

namespace Plu
{
    class RuntimeShaderCode;
    PLU_CLASS()
    class RuntimeShaderManager : public IShaderManager
    {
        REFLECTION_BODY_RUNTIMESHADERMANAGER()
    private:
        GameHashMap<UInt64, TOwningPointer<RuntimeShaderCode>> mShaderCodes;
    public:
        RuntimeShaderManager();
        virtual ~RuntimeShaderManager() override;

        void ShaderCodeScan();

        bool ShaderCodeExists(PluUUID uuid) override;
        DynamicArray<TUsePointer<ShaderProgram>> *GetRenderableShaderPrograms() override;
        TUsePointer<IShaderCode> GetShaderCode(PluUUID uuid) override;
        TUsePointer<ShaderProgram> GetShaderProgram(PluUUID uuid) override;
        void LoadShader(PluUUID uuid) override;
    };
}



#endif //PLUENGINE_RUNTIMESHADERMANAGER_H
