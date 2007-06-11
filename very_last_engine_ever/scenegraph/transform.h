#pragma once

#include "node.h"
#include "math/matrix4x4.h"

namespace scenegraph
{
	class Transform : public Node
	{
	public:
		virtual NodeType getType() { return NODE_TRANSFORM; }
		
		virtual Matrhx4x4 getTransform() = 0;
	};
}
