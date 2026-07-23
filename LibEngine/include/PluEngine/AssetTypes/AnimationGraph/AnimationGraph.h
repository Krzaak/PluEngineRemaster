//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPH_H
#define PLUENGINE_ANIMATIONGRAPH_H

#include "PluEngine/NodeGraph/NodeGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimationGraph.generated.h"

namespace Plu
{
    struct TypeInfo;

    PLU_STRUCT(Abstract)
    struct PLU_API IAnimationGraphVariable
    {
        REFLECTION_BODY_IANIMATIONGRAPHVARIABLE()

        virtual void* GetData() = 0;
        [[nodiscard]] virtual JSON Serialize() = 0;
        virtual void DeSerialize(const JSON& jsonData, DeserializationContext* deserializationContext) = 0;
#ifdef PLU_ENGINE_EDITOR_BUILD
        // label is the value widget's ImGui label — the caller passes a hidden one ("##...") since the
        // variable's name already gets its own field in the panel.
        virtual bool DrawEditorControl(const String& label) = 0;
#endif

        String Name;
        String TypeName;
    };

    template <typename T>
    struct PLU_API AnimationGraphVariable : public IAnimationGraphVariable
    {
    private:
        T mValue;
    public:
        void DeSerialize(const JSON &jsonData, DeserializationContext *deserializationContext) override
        {
            // Deserialize straight into mValue — the old code wrote through a null void* and dropped
            // the result, so every loaded variable came back default-constructed.
            TypeSerializer<T>::Deserialize(deserializationContext, jsonData, &mValue);
        }

#ifdef PLU_ENGINE_EDITOR_BUILD
        bool DrawEditorControl(const String& label) override
        {
            return TypeSerializer<T>::EditorControl(&mValue, label);
        }
#endif

        void *GetData() override
        {
            return &mValue;
        }

        [[nodiscard]] JSON Serialize() override
        {
            return TypeSerializer<T>::Serialize(GetData());
        }
    };

    struct PLU_API AnimationGraphVariableFactory
    {
        using FactoryFunc = std::function<TOwningPointer<IAnimationGraphVariable>()>;
        static GameHashMap<String, FactoryFunc>& GetFactoryMap();

        template <typename T>
        static void RegisterType(const String& TypeName)
        {
            // Capture TypeName by value: the parameter is a reference that dangles once
            // RegisterType returns, but the factory runs much later (on "Add Variable").
            GetFactoryMap()[TypeName] = [TypeName]() -> TOwningPointer<IAnimationGraphVariable> {
                TOwningPointer<AnimationGraphVariable<T>> variable = CreateOwning<AnimationGraphVariable<T>>();
                variable->TypeName = TypeName;
                return variable;
            };
        }

        static TOwningPointer<IAnimationGraphVariable> CreateVariable(const String& TypeName)
        {
            if (!GetFactoryMap().Contains(TypeName)) {
                return nullptr;
            }
            return GetFactoryMap()[TypeName]();
        }
    };

    // Node graph that drives a skeleton's pose: samplers, blends, bone masks, blend spaces and
    // state transitions, evaluated into a Pose (see PluEngine/Animation/BoneTransform.h).
    //
    // The reusable node-graph machinery lives in PluEngine/NodeGraph/ (GraphNode/NodeGraph/links +
    // NodeGraphSerializer). This asset just specialises it to the animation domain: its nodes derive
    // AnimGraphNode. Nodes + links are (de)serialised by NodeGraphSerializer from the asset loader.
    PLU_STRUCT()
    struct PLU_API AnimationGraph : NodeGraph
    {
        REFLECTION_BODY_ANIMATIONGRAPH()

        // A graph is authored against one skeleton: node indices in every Pose flowing through it
        // are SkeletonPoseLayout indices of this skeleton, so a graph is only meaningful on meshes
        // sharing it. Used as the evaluation skeleton when the caller doesn't supply one (editor
        // preview), and as the reference for node-side authoring (bone pickers, masks).
        PLU_PROPERTY()
        TUsePointer<Skeleton> TargetSkeleton;

        DynamicArray<TOwningPointer<IAnimationGraphVariable>> Variables;

        // Whether this graph may drive `skeleton`. True when TargetSkeleton is unassigned (an
        // unconstrained graph — the callers then supply whatever skeleton they have) or when the
        // two are the same asset. False means the graph's node indices address a different
        // skeleton, so evaluating it would produce a garbage pose.
        [[nodiscard]] bool IsCompatibleWith(const TUsePointer<Skeleton>& skeleton) const;

        // The "Add Node" palette lists non-abstract subclasses of this.
        TypeInfo* GetNodeBaseType() override { return AnimGraphNode::GetStaticClass(); }

        // Evaluates the graph: finds the AnimOutputPoseNode and pulls its pose, which recursively
        // pulls upstream nodes across pose links (AnimGraphNode::EvaluateInputPose). Empty pose
        // when the graph has no output node.
        Pose Evaluate(AnimEvalContext& context);
    };
}

#endif //PLUENGINE_ANIMATIONGRAPH_H
