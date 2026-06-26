//
// Created by Plutex on 6/26/26.
//

#ifndef PLUENGINE_GIZMOUTILS_H
#define PLUENGINE_GIZMOUTILS_H

#include "PluEngine/Core.h"
#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"
#include "GizmoUtils.generated.h"

namespace Plu
{
    PLU_ENUM(PyNamespace=Plu)
    enum class GizmoOperationSpace
    {
        LOCAL,
        WORLD
    };

    PLU_ENUM(PyNamespace=Plu)
    enum class GizmoOperation
    {
        TRANSLATE,
        ROTATE,
        SCALE
    };

    inline ImGuizmo::MODE PluGizmoModeToImGuizmoMode(GizmoOperationSpace space)
    {
        switch (space) {
            case GizmoOperationSpace::LOCAL:
                return ImGuizmo::LOCAL;
                break;
            case GizmoOperationSpace::WORLD:
                return ImGuizmo::WORLD;
                break;
            default: ;
        }
        return ImGuizmo::WORLD;
    }

    inline ImGuizmo::OPERATION PluGizmoOperationToImGuizmoOperation(GizmoOperation op)
    {
        switch (op) {
            case GizmoOperation::TRANSLATE:
                return ImGuizmo::TRANSLATE;
                break;
            case GizmoOperation::ROTATE:
                return ImGuizmo::ROTATE;
                break;
            case GizmoOperation::SCALE:
                return ImGuizmo::SCALE;
                break;
        }
        return ImGuizmo::TRANSLATE;
    }
}

#endif //PLUENGINE_GIZMOUTILS_H
