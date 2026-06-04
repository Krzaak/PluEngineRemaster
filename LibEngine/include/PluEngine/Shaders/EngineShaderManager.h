//
// Created by Plutex on 6/5/26.
//

#ifndef PLUENGINE_ENGINESHADERMANAGER_H
#define PLUENGINE_ENGINESHADERMANAGER_H

#include "EngineShaderManager.generated.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
    class ShaderProgram;
    class IShaderCode;

    PLU_CLASS()
    class EngineShaderManager : public EngineObject
    {
        REFLECTION_BODY_ENGINESHADERMANAGER()
    private:
        GameHashMap<UInt64, TOwningPointer<IShaderCode>> mShaderCodes;
        GameHashMap<UInt64, TOwningPointer<ShaderProgram>> mShaderPrograms;

        HashSet<UInt64> mLoadedShaderPrograms;
    public:
        EngineShaderManager();
        virtual ~EngineShaderManager() override;

        //Getters
        TUsePointer<IShaderCode> GetShaderCode(PluUUID uuid);
        TUsePointer<ShaderProgram> GetShaderProgram(PluUUID uuid);
        HashSet<UInt64>* GetShaderProgramsInUse(PluUUID uuid);

        //Validation
        bool ShaderCodeExists(PluUUID uuid) const;
        bool ShaderProgramExists(PluUUID uuid) const;
        bool IsShaderProgramLoaded(PluUUID uuid) const;

        //Loading
        void RequestShaderProgramLoad(PluUUID uuid);
    };
}

#endif //PLUENGINE_ENGINESHADERMANAGER_H
