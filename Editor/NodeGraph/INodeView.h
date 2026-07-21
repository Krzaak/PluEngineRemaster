//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_INODEVIEW_H
#define PLUENGINE_INODEVIEW_H

#include "PluEngine/Core.h"
#include "imgui.h"

namespace Plu
{
	struct GraphNode;
	class NodeGraphEditor;

	// A node's on-canvas appearance. Register a subclass per node type for custom visuals ("smaczki");
	// unregistered nodes fall back to DefaultNodeView. Views are stateless singletons — the registry
	// stores raw pointers and never owns them.
	class INodeView
	{
	public:
		virtual ~INodeView() = default;
		virtual void Draw(GraphNode* node, NodeGraphEditor& editor) = 0;
	};

	// Default node rendering, top to bottom: header (display name), then the pin block (inputs in a
	// left column, outputs right-aligned in a right column), then the body — custom widgets, which
	// therefore always sit at the bottom of the node and never push the pins around. Split into
	// virtual hooks so a custom view can subclass and inject widgets (override DrawBody) without
	// re-implementing the node frame or the pin layout.
	class DefaultNodeView : public INodeView
	{
	public:
		void Draw(GraphNode* node, NodeGraphEditor& editor) override;

	protected:
		virtual void DrawHeader(GraphNode* node, NodeGraphEditor& editor);
		virtual void DrawPins(GraphNode* node, NodeGraphEditor& editor);
		// Drawn last, under the pins. Widgets wider than the pin block simply widen the node.
		virtual void DrawBody(GraphNode* node, NodeGraphEditor& editor) {}

		// Flat fill colour of the title bar. Override to colour-code a node family.
		virtual ImVec4 HeaderColor(GraphNode* node) { return ImVec4(0.16f, 0.34f, 0.58f, 1.0f); }
	};
}

#endif //PLUENGINE_INODEVIEW_H
