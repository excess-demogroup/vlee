#pragma once

#include "node.h"

namespace scenegraph
{
	class Scene : public Node
	{
	public:
		
		virtual NodeType getType() { return NODE_SCENE; }
	};
}