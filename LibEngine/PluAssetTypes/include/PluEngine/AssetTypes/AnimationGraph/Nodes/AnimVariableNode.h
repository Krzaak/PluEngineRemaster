//
// Created by Plutex on 7/23/26.
//

#ifndef PLUENGINE_ANIMVARIABLENODE_H
#define PLUENGINE_ANIMVARIABLENODE_H
#include "PluEngine/Core.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimVariableNode.generated.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/AssetTypes/Animation/AnimGraphInstance.h"

namespace Plu {
    struct IAnimationGraphVariable;

    // Reads one of the graph's variables and exposes its value on an output pin. The variable is
    // referenced by name (VariableName), the graph's stable variable key: it is serialized, and the
    // live `Variable` pointer is resolved from it after load (AnimationGraph::ResolveVariable
    // references) and written back to it before save (AnimationGraph::SyncVariableNodeNames).
    PLU_STRUCT()
    struct PLUASSETTYPES_API AnimVariableNode : AnimGraphNode
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

        // A variable node is a leaf (its "Value" output never reads another node), so it cannot start
        // a data cycle on its own; nodes that do compute from other data pins are guarded by the
        // cycle stack in GraphNode::ReadDataPin.
        bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
                                 const String& typeId, void* outValue) override {
            if (pinName != "Value") return false;

            // Prefer the live instance's value; fall back to the asset's own variable when there's
            // no instance (editor preview, e.g.) or the instance doesn't know this name. The instance
            // lives on the animation context, so a variable node read from a non-animation graph
            // simply gets the asset's authored defaults.
            auto* animContext = dynamic_cast<AnimEvalContext*>(&context);
            if (animContext && animContext->Instance) {
                TUsePointer<IAnimationGraphVariable> live =
                    animContext->Instance->GetVariables().Find(VariableName);
                if (live) return live->CopyValueTo(outValue, typeId);
            }

            return Variable ? Variable->CopyValueTo(outValue, typeId) : false;
        }
    };
}

#endif //PLUENGINE_ANIMVARIABLENODE_H
