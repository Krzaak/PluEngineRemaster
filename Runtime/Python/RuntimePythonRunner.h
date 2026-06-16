//
// Created by Plutex on 6/7/26.
//

#ifndef PLUENGINE_RUNTIMEPYTHONRUNNER_H
#define PLUENGINE_RUNTIMEPYTHONRUNNER_H

#include "PluEngine/Managers/PythonManager.h"
#include "RuntimePythonRunner.generated.h"

namespace Plu
{
    PLU_CLASS()
    class RuntimePythonRunner : public IPythonManager
    {
        REFLECTION_BODY_RUNTIMEPYTHONRUNNER()
    public:
        RuntimePythonRunner();
        virtual ~RuntimePythonRunner() override;

        bool RunScript(PluUUID uuid) override;
        bool RunScript(PathW path, PathW workDir, String args) override;

        void RunScript(Path scriptPath);
        void RunScripts(Path dir);
    };
}



#endif //PLUENGINE_RUNTIMEPYTHONRUNNER_H
