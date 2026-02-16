//
// Created by Plutex on 1/18/26.
//

#include "EngineClassTreePanel.h"

#include "UI/IconsFontAwesome7.h"

struct ReflectionTypeTree
{
	Plu::TypeInfo* type;
	DynamicArray<ReflectionTypeTree*> children;
};

void GatherParentType(Plu::TypeInfo* typeToCheck, DynamicArray<Plu::TypeInfo*>* tree)
{
	tree->PushBack(typeToCheck);
	if (!typeToCheck->BaseType) return;
	GatherParentType(typeToCheck->BaseType, tree);
}

void PopulateTree(UInt16 idx, ReflectionTypeTree* tree, DynamicArray<Plu::TypeInfo*>* classLine)
{
	tree->type = classLine->At(idx);
	if (classLine->Size() <= idx + 1) return;
	auto childIterator = tree->children.FindIf([&](ReflectionTypeTree* child) -> bool {
		return classLine->At(idx + 1)->TypeName == child->type->TypeName;
	});
	if (childIterator == tree->children.End()) {
		ReflectionTypeTree* child = new ReflectionTypeTree();
		tree->children.PushBack(child);
		PopulateTree(idx + 1, child, classLine);
	} else {
		PopulateTree(idx + 1, *childIterator, classLine);
	}
}

void CleanReflectionTreeData(ReflectionTypeTree* tree)
{
	for (auto child : tree->children) {
		CleanReflectionTreeData(child);
		delete child;
	}
}

void CreateReflectionClassTree(ReflectionTypeTree* startingPoint)
{
	startingPoint->type = nullptr;
	startingPoint->children.Clear();
	for (const auto& type : *Plu::TypeRegistry::GetInstance()->GetTypeMap()) {
		if (type.second->Type != Plu::TypeType::CLASS) continue;
		DynamicArray<Plu::TypeInfo*> deriveLine;
		deriveLine.Reserve(10);
		GatherParentType(type.second, &deriveLine);
		deriveLine.Reverse();
		PopulateTree(0, startingPoint, &deriveLine);
	}
}

void Tooltip(Plu::TypeInfo* type)
{
	if (ImGui::BeginItemTooltip()) {
		for (auto property : type->Properties) {
			ImGui::Text("%s - %s - Offset: %lu, Size: %lu",
				property->PropertyName.CStr(),
				property->PropertyTypeName.CStr(),
				property->PropertyOffset,
				property->PropertySize
			);
		}
		ImGui::EndTooltip();
	}
}

void FileNode(ReflectionTypeTree* type)
{
	ImGuiTreeNodeFlags fileFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGuiTreeNodeFlags fileFlags2 = fileFlags;
	// if (path == mSelectedFile) {
	// 	fileFlags2 |= ImGuiTreeNodeFlags_Selected;
	// }
	if (ImGui::TreeNodeEx(type->type->TypeName.CStr(), fileFlags2)) {
	}
	Tooltip(type->type);
}

void DirectoryNode(ReflectionTypeTree* type)
{
	ImGuiTreeNodeFlags dirFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
	if (ImGui::TreeNodeEx(type->type->TypeName.CStr(), dirFlags))
	{
		for (auto child : type->children) {
			if (!child->children.IsEmpty()) {
				DirectoryNode(child);
			} else {
				FileNode(child);
			}
		}
		ImGui::TreePop();
	}
	Tooltip(type->type);
}

Plu::String Plu::EngineClassTreePanel::GetPanelName()
{
	return ICON_FA_TREE " Reflection Class Tree";
}

void Plu::EngineClassTreePanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		ReflectionTypeTree engineObject;
		CreateReflectionClassTree(&engineObject);
		DirectoryNode(&engineObject);
		CleanReflectionTreeData(&engineObject);
	}
	EndPanel();
}

void Plu::EngineClassTreePanel::OnHide()
{
}

void Plu::EngineClassTreePanel::OnShow()
{
	SetCanClose(true);
}
