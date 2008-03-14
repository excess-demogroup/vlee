#pragma once

namespace scenegraph
{
	class DrawableNode : public Node
	{
	public:
		DrawableNode(std::string name) : Node(name) { }
		NodeType getType() { return NODE_DRAWABLE; }
		
		virtual bool isTransparent() = 0;
		virtual void draw() = 0;
	};
}
