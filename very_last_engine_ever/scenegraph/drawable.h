#pragma once

namespace scenegraph
{
	class Drawable : public Node
	{
	public:
		Drawable(std::string name) : Node(name) { }
		NodeType getType() { return NODE_DRAWABLE; }
		
		virtual bool isTransparent() = 0;
		virtual void draw() = 0;
	};
}
