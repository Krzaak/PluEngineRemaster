//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_INODEVIEW_H
#define PLUENGINE_INODEVIEW_H

#include "PluEngine/Core.h"

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

	// Default node rendering: header (display name) + input/output pins. Split into virtual hooks so
	// a custom view can subclass and inject extra widgets (override DrawBody) without re-implementing
	// the node frame or pin layout.
	class DefaultNodeView : public INodeView
	{
	public:
		void Draw(GraphNode* node, NodeGraphEditor& editor) override;

	protected:
		virtual void DrawHeader(GraphNode* node, NodeGraphEditor& editor);
		virtual void DrawBody(GraphNode* node, NodeGraphEditor& editor) {}
		virtual void DrawPins(GraphNode* node, NodeGraphEditor& editor);
	};
}

#endif //PLUENGINE_INODEVIEW_H
