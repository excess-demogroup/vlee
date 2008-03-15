#pragma once

#include "node.h"
#include "camera.h"
#include "target.h"

namespace scenegraph
{
	class Scene : public Node
	{
	public:
		Scene(std::string name) : Node(name) {}
		
		virtual NodeType getType() { return NODE_SCENE; }
		
		Camera *findCamera(const std::string &name)
		{
			return findNodeByType<Camera, NODE_CAMERA>(name);
		}
		
		Target *findTarget(std::string name)
		{
			return findNodeByType<Target, NODE_TARGET>(name);
		}
		
	private:
		template <typename T, NodeType t>
		T *findNodeByType(const std::string &name)
		{
			// find node
			Node *node = findChild(name);
			if (NULL == node) return NULL;
			
			// type check
			if (node->getType() != t) return NULL;
			return reinterpret_cast<T*>(node);
		}
	};
}