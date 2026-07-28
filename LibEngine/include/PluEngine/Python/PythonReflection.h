//
// Created by Plutex on 7/28/26.
//

#ifndef PLUENGINE_PYTHONREFLECTION_H
#define PLUENGINE_PYTHONREFLECTION_H

#include "PluEngine/Reflection/ClassPointer.h"

namespace Plu
{
    PLU_FUNCTION()
    bool PLU_API IsPythonClassDerivedOf(const pybind11::type &child, pybind11::type base);
}

#endif //PLUENGINE_PYTHONREFLECTION_H
