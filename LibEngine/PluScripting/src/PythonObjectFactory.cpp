//
// Created by Plutex on 8/12/26.
//

#include "PluEngine/Scripting/PythonObjectFactory.h"

#include "PluEngine/Scripting/PythonPointers.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"

void Plu::InstallPythonObjectFactory()
{
    EngineObjectManager::PythonObjectFactory = [](pybind11::type type) {
        return OwnerFromPython<EngineObject>(type);
    };
}
