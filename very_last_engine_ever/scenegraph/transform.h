#pragma once

#include "node.h"
#include "math/matrix4x4.h"

namespace scenegraph
{
	class Transform : public Node
	{
	public:
		Transform(std::string name) : Node(name) {}
		
		virtual NodeType getType() { return NODE_TRANSFORM; }
	};
}
