#pragma once

#include <string>

namespace scenegraph
{
	enum NodeType
	{
		NODE_SCENE,
		NODE_CAMERA,
		NODE_LIGHT,
		NODE_TRANSFORM,
		NODE_DRAWABLE,
	};

	class Node
	{
	public:

		Node(std::string name) : parent(NULL) { }
		
		virtual ~Node();

		void addChild(Node *node)
		{
			children.push_back(node);
		}
		
		Node *getParent() const
		{
			return parent; // who's your daddy?!
		}

		virtual NodeType getType() = 0;

	private:
		std::string name;
		
		Node *parent;
		std::list<Node*> children;
	};
}
