#pragma once

#include "node.h"

namespace scenegraph
{
	class Scene : public Node
	{
	public:
		Scene(std::string name) : Node(name) {}

		virtual NodeType getType() { return NODE_SCENE; }
	};
}