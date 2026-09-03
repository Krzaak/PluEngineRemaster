//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_PYTHONOBJECTFACTORY_H
#define PLUENGINE_PYTHONOBJECTFACTORY_H
#include "PluEngine/Core.h"

namespace Plu
{
    // Teaches EngineObjectManager how to instantiate a Python-defined engine class. The object
    // manager lives in PluCore, below scripting, so it holds a callback instead of calling into
    // pybind11 itself. Call once during engine init, before any Python type is instantiated.
    PLUSCRIPTING_API void InstallPythonObjectFactory();
}

#endif //PLUENGINE_PYTHONOBJECTFACTORY_H
