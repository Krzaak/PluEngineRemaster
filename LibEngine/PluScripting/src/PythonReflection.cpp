//
// Created by Plutex on 7/28/26.
//

#include "PluEngine/Scripting/PythonReflection.h"

bool Plu::IsPythonClassDerivedOf(const pybind11::type &child, pybind11::type base)
{
    TypeInfo* pythonType = TypeRegistry::GetInstance()->GetTypeOfName(GetTypeNameFromPython(child));
    if (pythonType == nullptr)
        return false;

    TypeInfo* baseType = TypeRegistry::GetInstance()->GetTypeOfName(GetTypeNameFromPython(base));
    if (baseType == nullptr)
        return false;

    return pythonType->IsDerivedOfOrSame(baseType);
}
