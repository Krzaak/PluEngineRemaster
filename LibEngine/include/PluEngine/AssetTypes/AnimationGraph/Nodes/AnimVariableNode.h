//
// Created by Plutex on 7/23/26.
//

#ifndef PLUENGINE_ANIMVARIABLENODE_H
#define PLUENGINE_ANIMVARIABLENODE_H
#include "PluEngine/Core.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimVariableNode.generated.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"

namespace Plu {
    struct IAnimationGraphVariable;

    // Reads one of the graph's variables and exposes its value on an output pin. The variable is
    // referenced by name (VariableName), the graph's stable variable key: it is serialized, and the
    // live `Variable` pointer is resolved from it after load (AnimationGraph::ResolveVariable
    // references) and written back to it before save (AnimationGraph::SyncVariableNodeNames).
    PLU_STRUCT()
    struct PLU_API AnimVariableNode : AnimGraphNode
    {
        REFLECTION_BODY_ANIMVARIABLENODE()

        // Persistent reference key — which graph variable this node reads.
        PLU_PROPERTY()
        String VariableName;

        // Resolved live variable (never serialized). Null when the referenced variable is missing
        // (deleted, or a load where the name no longer matches any variable).
        TUsePointer<IAnimationGraphVariable> Variable;

        void BuildPins() override {
            // The pin name is deliberately constant, NOT the variable name: links reference pins by
            // name, so a stable name keeps existing links intact when the variable is renamed. The
            // variable's identity shows in the node title (GetDisplayName) instead.
            AddPin("Value", EPinDirection::Output, EPinCategory::Data,
                   Variable ? Variable->PinTypeId : String());
        }

        String GetDisplayName() override {
            if (Variable) return Variable->Name;
            return VariableName.IsEmpty() ? String("Variable") : VariableName;
        }
    };
}

#endif //PLUENGINE_ANIMVARIABLENODE_H
